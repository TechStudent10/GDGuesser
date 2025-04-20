import "dotenv/config"

import express from "express"
import { createServer } from "http"
import * as fs from "node:fs"
import * as path from "node:path"

import sqlite3 from "sqlite3"
import { open } from "sqlite"

const router = express()
const httpServer = createServer(router)
router.use(express.json());

// router.use((req, res, next) => {
//     console.log(games)
//     next();
// })

const lvlStats = fs.readFileSync(path.join(__dirname, "..", "resources", "LevelStats.dat")).toString()
const unsortedLevelIds = lvlStats.split(",").filter((_, index) => index % 2 === 0).map(val => parseInt(val));

// taken from https://github.com/Alphalaneous/Random-Tab/blob/main/src/RandomLayer.hpp#L27
const ID_CUTOFFS: {[key: string]: number[]} = {
    "1.0": [128, 3785],
    "1.1": [3786, 13519],
    "1.2": [13520, 66513],
    "1.3": [66514, 122780],
    "1.4": [122781, 184402],
    "1.5": [184403, 422703],
    "1.6": [422704, 835700],
    "1.7": [835701, 1628620],
    "1.8": [1628621, 2804784],
    "1.9": [2804785, 11040708],
    "2.0": [11040709, 28350553],
    "2.1": [28360554, 97454811],
    "2.2": [97454812, 130000000]
}

const levelIds = (() => {
    const result: {[key: string]: number[]} = {}
    Object.keys(ID_CUTOFFS).forEach(update => {
        result[update] = unsortedLevelIds.filter((id) => id >= ID_CUTOFFS[update][0] && id <= ID_CUTOFFS[update][1])
    })
    return result
})()

async function openDB() {
    return await open({
        filename: process.env.DATABASE_FILE || "./database.db",
        driver: sqlite3.Database
    })
}

(async () => {
    const db = await openDB();
    db.migrate()
})()

type LevelDate = {
    year: number
    month: number
    day: number
}

enum GameMode {
    Normal,
    Hardcore
}

type GameOptions = {
    mode: GameMode
}

type Game = {
    accountId: number
    currentLevelId: number
    options: GameOptions
}

const games: {[key: string]: Game} = {}

function stringToLvlDate(str: string): LevelDate {
    const split = str.split("-").map(val => parseInt(val))
    return {
        year: split[0],
        month: split[1],
        day: split[2]
    }
}

// scoring works as indicated in the Doggie video
// if the player is within a week of the correct answer, they get the maximum amount of points
// (500 for normal, 600 for hardcore)
// if they are not, we take off one point for every day off, to a mimimum of 0 points

function calcScore(guess: LevelDate, correct: LevelDate, mode: GameMode): number {
    const limit = mode === GameMode.Normal ? 500 : 600;

    const guessDate = new Date(guess.year, guess.month - 1, guess.day)
    const correctDate = new Date(correct.year, correct.month - 1, correct.day)

    const diffDays = Math.ceil(Math.abs(guessDate.getTime() - correctDate.getTime()) / (1000 * 3600 * 24))

    if (diffDays <= 7) {
        return limit
    }

    return Math.max(limit - diffDays, 0)
}

function getRandomElement<T>(arr: Array<T>) {
    return arr[Math.floor((Math.random() * arr.length) % arr.length)]
}

const getRandomLevelId = () => getRandomElement(levelIds[getRandomElement(Object.keys(ID_CUTOFFS))])

function getDashAuthUrl() {
    let str = process.env.DASHAUTH_URL
    return str?.endsWith('/') ?
        str.slice(0, -1) :
        str
}

async function checkToken(token: string) {
    const req = await fetch(`${getDashAuthUrl()}/api/v1/verify`, {
        method: "POST",
        body: JSON.stringify({
            token
        }),
        headers: {
            "Content-Type": "application/json"
        }
    })
    return req 
}

router.get("/", (req, res) => {
    res.send("we are up and running! go get guessing!")
})

router.post("/", async (req, res, next) => {
    const token = req.headers.authorization || "";
    const daResp = await checkToken(token)

    if (daResp.status === 401) {
        res.status(401).send("invalid token")
        return
    }

    next()
})

router.post("/start-new-game", async (req, res) => {
    const token = req.headers.authorization || "";
    const daResp = await checkToken(token)

    // if (daResp.status === 401) {
    //     res.status(401).send("invalid token")
    //     return
    // }

    const data = (await daResp.json())["data"]
    if (Object.keys(games).includes(data["id"])) {
        res.status(400).send("game already in progress")
        return
    }

    const id = getRandomLevelId()
    games[data["id"]] = {
        accountId: data["id"],
        currentLevelId: id,
        options: req.body["options"]
    }

    res.json({
        level: id,
        game: games[data["id"]]
    })
})

// `date` is in the format: `year-month-day`, i.e.: "2013-08-13"

router.post("/guess/:date", async (req, res) => {
    const token = req.headers.authorization || ""
    const daResp = await checkToken(token)

    if (daResp.status === 401) {
        res.status(401).send("invalid token")
        return
    }
    const data = (await daResp.json())["data"]
    if (!Object.keys(games).includes(data["id"].toString())) {
        res.status(404).send("game does not exist. did you mean to call /start-new-game?")
        return
    }

    const { date } = req.params
    const levelId = games[data["id"]].currentLevelId

    const correctDate: string = (
    await (
        await fetch(`https://history.geometrydash.eu/api/v1/date/level/${levelId}/`)).json()
    )["approx"]["estimation"]
    .split("T")[0]

    const score = calcScore(stringToLvlDate(date), stringToLvlDate(correctDate), games[data["id"]].options.mode)

    const db = await openDB()
    await db.run(`
        INSERT INTO scores (account_id, username, total_score, icon_id)
        VALUES (?, ?, ?, ?)    
        ON CONFLICT (account_id) DO
        UPDATE SET username = ?, total_score = total_score + ?
    `, data["id"], data["username"], score, 1, data["username"], score)

    delete games[data["id"]]

    res.json({
        score,
        correctDate
    })
})

router.post("/endGame", async (req, res) => {
    const token = req.headers.authorization || ""
    const daResp = await checkToken(token)

    if (daResp.status === 401) {
        res.status(401).send("invalid token")
        return
    }
    const data = (await daResp.json())["data"]
    if (!Object.keys(games).includes(data["id"].toString())) {
        res.status(404).send("game does not exist")
        return
    }

    delete games[data["id"]]

    res.status(200).send()
})

router.get("/account/:id", async (req, res) => {
    const db = await openDB()

    const response = await db.get(`
        SELECT * FROM scores
        WHERE account_id = ?     
    `, req.params.id)

    if (!response) {
        res.status(404).send("no account found")
        return
    }

    res.json(
        response
    )
})

router.get("/leaderboard", async (req, res) => {
    const db = await openDB()
    const results = await db.all(`
        SELECT * FROM scores
        ORDER BY total_score DESC
        LIMIT 100
    `)
    res.json(results)
})

router.use("/dashauth", async (req, res, next) => {
    const resp = await (await fetch(`${getDashAuthUrl()}${req.path.replace("dashauth", "")}`, {
        body: req.body,
        headers: Object(req.headers),
        method: req.method
    })).json()

    // console.log(resp)

    res.json(resp)
})

const port = process.env.PORT || 8000

console.log(`listening on port ${port}`)
httpServer.listen(port)
