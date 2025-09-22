import WebSocket from "ws"
import { Server as HTTPServer } from "http"

import { UserToken, checkToken, GameMP, GameOptions, generateCode, GameMode, getUserByID, User, getRandomLevelId, stringToLvlDate, calcScore, LevelDate } from "../shared"
import { DuelCreated, DuelEnded, DuelJoined, DuelResults, DuelUpdated, OpponentGuessed, ReceiveDuel, RoundStarted } from "./server"

type ClientMessage = {
    eventName: string
    token: string
    payload: {
        [key: string]: unknown
    }
}

function createEvent<T>(eventName: string, data: T) {
    console.log("sending event", eventName)
    return JSON.stringify({
        eventName,
        payload: data
    })
}

function broadcast(code: string, data: string) {
    Object.values(sockets[code]).forEach(socket => {
        socket.send(data)
    })
}

function duelUpdated(code: string) {
    broadcast(
        code,
        createEvent<DuelUpdated>(
            "duel updated",
            { duel: games[code] }
        )
    )
}

const games: {
    [key: string]: GameMP
} = {}

const sockets: {
    [key: string]: {
        [key: number]: WebSocket
    }
} = {}

// used to hold round scores before
// final scoring
const tempScores: {
    [key: string]: {
        [key: number]: number
    }
} = {}
const tempDates: {
    [key: string]: {
        [key: number]: LevelDate
    }
} = {}

function deleteDuel(joinCode: string) {
    try {
        delete games[joinCode]
        delete sockets[joinCode]
        delete tempScores[joinCode]
        delete tempDates[joinCode]
    } catch (e) {
        console.error(`could not delete duel ${joinCode}; reason: ${e}`)
    }
}

function getGameByPlayer(account_id: number) {
    try {
        return Object.values(games).filter(game => Object.keys(game.players).includes(account_id.toString() || ""))[0]
    } catch (e) {
        console.error(e)
        return undefined
    }
}

const handlers: {
    [key: string]: (socket: WebSocket, userData: UserToken, data: {
        [key: string]: any
    }) => void
} = {
    "create duel": (socket, user, payload) => {
        const options = (payload as {
            options: GameOptions
        }).options

        const joinCode = generateCode()
        const game: GameMP = {
            currentLevelId: 0,
            joinCode,
            options,
            players: {},
            scores: {},
            playersReady: []
        }

        games[joinCode] = game
        sockets[joinCode] = {}
        tempScores[joinCode] = {}
        tempDates[joinCode] = {}

        socket.send(createEvent<DuelCreated>(
            "duel created",
            {
                joinCode
            }
        ))

        console.log(`user ${user.username} just created a duel with code ${joinCode}`)
    },
    "join duel": async (socket, user, payload) => {
        const joinCode: string = payload["joinCode"]

        if (!games[joinCode]) return; // TODO: send error (caused by user clicking join without entering a code)

        if (Object.keys(games[joinCode].players).includes(user.account_id.toString())) {
            // TODO: send error
            return
        }

        if (Object.keys(games[joinCode].players).length >= 2) {
            // TODO: send error
            return
        }        

        // this is cursed and bad practive i know
        // BUT we know the user exists because they need to
        // go through /login which automatically inserts the
        // user into the DB
        // i better not regret this

        // this could prolly be abused to crash the server -- Vinster
        games[joinCode].players[user.account_id] = (await getUserByID(user.account_id)) as User
        sockets[joinCode][user.account_id] = socket

        socket.send(
            createEvent<DuelJoined>(
                "duel joined", {}
            )
        )

        // give the client sometime to open the popup, so the update event can be broadcasted
        // (by sometime i mean like fractions of a second because computers are fast)
        setTimeout(() => {
            duelUpdated(joinCode)
        }, 500)
    },
    "toggle ready": (socket, user, payload) => {
        const duel = getGameByPlayer(user.account_id) as GameMP
        if (duel.playersReady.includes(user.account_id)) {
            duel.playersReady.splice(
                duel.playersReady.indexOf(user.account_id),
                1
            )
        } else {
            duel.playersReady.push(user.account_id)
            // if the player isn't ready give them time to unready up
            setTimeout(() => {
                if (duel.playersReady.length < 2) return
                const lvlId = getRandomLevelId(duel.options.versions, 0)
                duel.currentLevelId = lvlId
                if (Object.keys(duel.scores).length === 0) {
                    Object.values(duel.players).forEach(player => {
                        duel.scores[player.account_id] = 650
                    })
                }
                broadcast(duel.joinCode, createEvent<RoundStarted>(
                    "round started",
                    {
                        options: duel.options,
                        levelId: lvlId
                    }
                ))
                duel.playersReady = []
                games[duel.joinCode] = duel
            }, 1000)
        }

        games[duel.joinCode] = duel
        duelUpdated(duel.joinCode)
    },
    "submit guess": async (socket, user, payload) => {
        const date = (payload as {
            date: LevelDate
        }).date

        const duel = getGameByPlayer(user.account_id) as GameMP
        let correctDate: string;
        try {
            const res = await fetch(`https://history.geometrydash.eu/api/v1/date/level/${duel.currentLevelId}/`)
            if (!res.ok) throw new Error("History server returned non-OK status")

            const json = await res.json()
            correctDate = json["approx"]["estimation"].split("T")[0]
        } catch (err) {
            // TODO: send this error to client and cancel duel
            // I trust you to add that error event --Vinster
            console.error(`gdhistory offline`);
            return
        }

        const score = calcScore(date, stringToLvlDate(correctDate), duel.options)
        tempScores[duel.joinCode][user.account_id] = score[0]
        tempDates[duel.joinCode][user.account_id] = date

        let opponentAccId = Object.keys(sockets[duel.joinCode]).filter(accId => accId !== user.account_id.toString())[0]
        sockets[duel.joinCode][parseInt(opponentAccId)].send(
            createEvent<OpponentGuessed>(
                "opponent guessed",
                {}
            )
        )

        if (Object.keys(tempScores[duel.joinCode]).length === 2) {
            const scores = Object.values(tempScores[duel.joinCode])
            
            const bestScore = Math.max(...scores)
            const worstScore = Math.min(...scores)
            const difference = bestScore - worstScore

            const playerAffected = Object.keys(tempScores[duel.joinCode])[
                scores.indexOf(worstScore)
            ]
            if (difference !== 0) {
                duel.scores[playerAffected] -= difference
            }

            if (duel.scores[playerAffected] <= 0) {
                broadcast(
                    duel.joinCode,
                    createEvent<DuelEnded>(
                        "duel ended",
                        {
                            duel,
                            loser: parseInt(playerAffected),
                            winner: parseInt(Object.keys(tempScores[duel.joinCode])[
                                scores.indexOf(bestScore)
                            ])
                        }
                    )
                )

                return
            }

            broadcast(
                duel.joinCode,
                createEvent<DuelResults>(
                    "duel results",
                    {
                        scores: tempScores[duel.joinCode],
                        totalScores: duel.scores,
                        guessedDates: tempDates[duel.joinCode],
                        correctDate: stringToLvlDate(correctDate)
                    }
                )
            )

            games[duel.joinCode] = duel
            tempScores[duel.joinCode] = {}
            tempDates[duel.joinCode] = {}
            duelUpdated(duel.joinCode)
        }
    },
    "get duel": (socket, user, _) => {
        const duel = getGameByPlayer(user.account_id) as GameMP
        socket.send(
            createEvent<ReceiveDuel>(
                "receive duel",
                {
                    duel
                }
            )
        )
    },
    "forfeit": (socket, user, payload) => {
        try {
            const duel = getGameByPlayer(user.account_id) as GameMP
            if (duel.playersReady.includes(user.account_id)) {
                duel.playersReady.splice(
                    duel.playersReady.indexOf(user.account_id),
                    1
                )
            }
            // this is only true if the duel has started
            if (Object.keys(duel.scores).length !== 0) {
                broadcast(
                    duel.joinCode,
                    createEvent<DuelEnded>(
                        "duel ended",
                        {
                            duel,
                            loser: user.account_id,
                            winner: parseInt(Object.keys(duel.players).filter(
                                accId => accId !== user?.account_id.toString()
                            )[0])
                        }
                    )
                )

                console.log(`duel ${duel.joinCode} ended`)
                return
            }
            
            if (Object.keys(duel.players).length === 0) {
                deleteDuel(duel.joinCode)
                console.log(`deleted duel ${duel.joinCode}`)
                return
            }
            games[duel.joinCode] = duel
            duelUpdated(duel.joinCode)
        } catch (e) {
            console.error(e)
        }
    }
}

function setupMP(server: HTTPServer) {
    const ws = new WebSocket.Server({ server })

    ws.on("connection", (socket) => {
        let user: User | null = null

        console.log('wow connection!')

        socket.on("close", () => {
            console.log("oh we closed!")

            if (!user) {
                // we can safely assume they aren't in a duel
                return
            }

            console.log(`player ${user.username} disconnected`)

            try {
                const duel = getGameByPlayer(user.account_id) as GameMP
                delete duel.players[user.account_id.toString()]
                delete sockets[duel.joinCode][user.account_id]
                if (duel.playersReady.includes(user.account_id)) {
                    duel.playersReady.splice(
                        duel.playersReady.indexOf(user.account_id),
                        1
                    )
                }
                // this is only true if the duel has started
                if (Object.keys(duel.scores).length !== 0) {
                    broadcast(
                        duel.joinCode,
                        createEvent<DuelEnded>(
                            "duel ended",
                            {
                                duel,
                                loser: user.account_id,
                                winner: parseInt(Object.keys(duel.players).filter(
                                    accId => accId !== user?.account_id.toString()
                                )[0])
                            }
                        )
                    )

                    console.log(`duel ${duel.joinCode} ended`)
                    return
                }
                
                if (Object.keys(duel.players).length === 0) {
                    deleteDuel(duel.joinCode)
                    console.log(`deleted duel ${duel.joinCode}`)
                    return
                }
                games[duel.joinCode] = duel
                duelUpdated(duel.joinCode)
            } catch (e) {
                console.error(e)
            }
        })

        socket.on("message", async (dataStr) => {
            const data = JSON.parse(dataStr.toString()) as ClientMessage

            const userData = await checkToken(data.token)
            if (!userData.user) {
                // honestly this should send the error to the client
                // but i am lazy rn so ill do it later when i add
                // the error event
                console.error(`invalid token: ${userData.error}`)
                return
            }

            if (!user) {
                user = await getUserByID(userData.user.account_id) as User
            }

            if (!Object.keys(handlers).includes(data.eventName)) {
                console.error(`no handler exists for event "${data.eventName}"`)
                return
            }
            
            handlers[data.eventName](socket, userData.user, data.payload)
        })
    })
}

export default setupMP
