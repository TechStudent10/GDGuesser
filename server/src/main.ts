import "dotenv/config"

import express from "express"
import { createServer } from "http"
import * as fs from "node:fs"
import * as path from "node:path"

import sqlite3 from "sqlite3"
import { open } from "sqlite"

import * as jwt from "jsonwebtoken"

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

const VERSION_WEIGHTS: {[key: string]: number[]} = {
    //[0] = Normal, [1] = whatever new difficulty you want to add (normal by default)
    "1.0": [0.025],
    "1.1": [0.025],
    "1.2": [0.04],
    "1.3": [0.045],
    "1.4": [0.045],
    "1.5": [0.065],
    "1.6": [0.065],
    "1.7": [0.075],
    "1.8": [0.075],
    "1.9": [0.12],
    "2.0": [0.12, 0.0],
    "2.1": [0.15, 0.27],
    "2.2": [0.15]
}

const SECRET_KEY = process.env.SECRET_KEY || ""

const levelIds = (() => {
    const result: {[key: string]: number[]} = {}
    const start = Date.now()
    Object.keys(ID_CUTOFFS).forEach(update => {
        result[update] = unsortedLevelIds.filter((id) => id >= ID_CUTOFFS[update][0] && id <= ID_CUTOFFS[update][1])
    })
    console.log(`id cutoffs took ${Date.now() - start}ms`)
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

type UserToken = {
    username: string
    account_id: number
}

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
    mode: GameMode,
    versions: string[]
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
// returns the score and their accuracy

function calcScore(guess: LevelDate, correct: LevelDate, options: GameOptions): number[] {
    const limit = options.mode === GameMode.Normal ? 500 : 600;

    const guessDate = new Date(guess.year, guess.month - 1, guess.day)
    const correctDate = new Date(correct.year, correct.month - 1, correct.day)

    const diffDays = Math.ceil(Math.abs(guessDate.getTime() - correctDate.getTime()) / (1000 * 3600 * 24))
    const score = diffDays <= 7 ? limit : Math.max(limit - (diffDays - 7), 0)
    const accuracy = score / limit * 100

    return [score, accuracy]
}

function getRandomElement<T>(arr: Array<T>) {
    return arr[Math.floor((Math.random() * arr.length) % arr.length)]
}

const getRandomLevelId = (allowedVersions: string[], weightList: number) => {
    let versions = Object.keys(ID_CUTOFFS)
    if (allowedVersions.length) {
        versions = versions.filter(version => allowedVersions.includes(version))
    }
    const getWeight = (version: string) => {
        const arr = VERSION_WEIGHTS[version] || []
        return arr[weightList] ?? arr[0] ?? 0
    }
    const totalWeight = versions.reduce(
        (sum, version) => sum + getWeight(version), 0
    )
    let random = Math.random() * totalWeight
    const chosenVersion = versions.find(v => {
      random -= getWeight(v)
      return random <= 0
    }) || versions[0]
    return getRandomElement(levelIds[chosenVersion])
}

// function getDashAuthUrl() {
//     let str = process.env.DASHAUTH_URL
//     return str?.endsWith('/') ?
//         str.slice(0, -1) :
//         str
// }

async function checkToken(token: string): Promise<{
    user: UserToken | undefined,
    error: any
}> {
    try {
        const req = jwt.verify(token, SECRET_KEY) as UserToken
        return {
            user: req,
            error: ""
        }
    } catch (error) {
        return {
            user: undefined,
            error
        }
    }
}

async function getUser(account_id: string | number) {
    const db = await openDB()

    const response = await db.get(`
        SELECT * FROM scores
        WHERE account_id = ?     
    `, account_id)

    if (!response) {
        return undefined
    }

    return response
}

async function submitScore(mode: GameMode, user: UserToken, score: number, accuracy: number) {
    const max_score = mode === GameMode.Normal ? 500 : 600;

    const db = await openDB()
    await db.run(`
        INSERT INTO scores (
            account_id,
            username,
            total_score,
            icon_id,
            accuracy,
            max_score,
            total_normal_guesses,
            total_hardcore_guesses
        )
        VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        ON CONFLICT (account_id) DO
        UPDATE SET
            username = ?,
            total_score = total_score + ?,
            accuracy = (accuracy + ?) / 2,
            max_score = max_score + ?,
            total_normal_guesses = total_normal_guesses + ?,
            total_hardcore_guesses = total_hardcore_guesses + ?;
    `,
        user.account_id,
        user.username,
        score,
        1,
        accuracy,
        max_score,
        mode === GameMode.Normal ? 1 : 0,
        mode === GameMode.Hardcore ? 1 : 0,
        user.username,
        score,
        accuracy,
        max_score,
        mode === GameMode.Normal ? 1 : 0,
        mode === GameMode.Hardcore ? 1 : 0,   
    )
}

router.get("/", (req, res) => {
    res.send("we are up and running! go get guessing!")
})

router.get("/getStats", async (req, res) => {
    const db = await openDB();
    res.json({
        onlineCount: Object.keys(games).length,
        userCount: (await db.get("SELECT COUNT(*) as userCount from scores"))["userCount"],
        totalScore: (await db.get("SELECT SUM(total_score) as totalScore FROM scores"))["totalScore"],
        overallAccuracy:( await db.get(`
            SELECT
            SUM(total_score) * 1.0
            / SUM(max_score) * 100.0 as overallAccuracy
            FROM scores;
        `))["overallAccuracy"]
    })
})

router.post("/login", async (req, res) => {
    const token = req.headers.authorization || ""
    const account_id = req.body["account_id"]
    const daResp = await fetch(`https://argon.globed.dev/v1/validation/check_strong?authtoken=${token}&account_id=${account_id}`)
    
    if (daResp.status !== 200) {
        res.status(401).send(daResp.statusText)
        return
    }

    const data = await daResp.json()
    
    if (!data["valid_weak"]) {
        res.status(401).send(`Invalid token: ${data["cause"]}`)
        return
    }

    
    const db = await openDB()
    
    await db.run(`
        INSERT INTO scores (account_id, username, icon_id, color1, color2, color3, total_score, accuracy, max_score, total_normal_guesses, total_hardcore_guesses)
        VALUES (?, ?, ?, ?, ?, ?, 0, 0, 0, 0, 0)    
        ON CONFLICT (account_id) DO
        UPDATE SET username = ?, icon_id = ?, color1 = ?, color2 = ?, color3 = ?
        `, 
        account_id, data["username"],
        req.body["icon_id"], req.body["color1"], req.body["color2"], req.body["color3"],
        data["username"],
        req.body["icon_id"], req.body["color1"], req.body["color2"], req.body["color3"]
    )

    const tokenInfo: UserToken = {
        account_id: account_id,
        username: data["username"]
    }
    const jwtToken = jwt.sign(tokenInfo, SECRET_KEY)

    res.status(200).json({
        user: await getUser(account_id),
        token: jwtToken
    })
})

router.post("/start-new-game", async (req, res) => {
    const token = req.headers.authorization || "";
    const data = await checkToken(token)
    
    if (!data.user) {
        res.status(401).send(`Invalid token: ${data.error}`)
        return
    }

    const account_id = data.user.account_id
    const gameExists = Object.keys(games).includes(String(account_id))

    if (gameExists) {
        submitScore(games[account_id].options.mode, data.user, 0, 0)
    }

    const options = req.body["options"] as GameOptions
    let list: number;
    switch(options.mode) {
        case GameMode.Normal: list = 0; break;
        case GameMode.Hardcore: list = 0; break;
        default: list = 0;
    }
    const id = getRandomLevelId(options.versions, list)
    games[account_id] = {
        accountId: account_id,
        currentLevelId: id,
        options: options
    }

    res.json({
        level: id,
        game: games[account_id],
        gameExists
    })
})

// `date` is in the format: `year-month-day`, i.e.: "2013-08-13"

router.post("/guess/:date", async (req, res) => {
    const token = req.headers.authorization || ""
    const data = await checkToken(token)
    
    if (!data.user) {
        res.status(401).send(`Invalid token: ${data.error}`)
        return
    }

    const account_id = data.user.account_id

    if (!Object.keys(games).includes(account_id.toString())) {
        res.status(404).send("game does not exist. did you mean to call /start-new-game?")
        return
    }

    
    const { date } = req.params
    const levelId = games[account_id].currentLevelId
    
    const correctDate: string = (
    await (
        await fetch(`https://history.geometrydash.eu/api/v1/date/level/${levelId}/`)).json()
    )["approx"]["estimation"]
    .split("T")[0]
    
    
    const scoreResult = calcScore(stringToLvlDate(date), stringToLvlDate(correctDate), games[account_id].options)
    const score = scoreResult[0]
    const accuracy = scoreResult[1]
    
    const gameMode = games[account_id].options.mode
    
    // only submit if all versions are selected
    if (games[account_id].options.versions.length === Object.keys(ID_CUTOFFS).length) {
        submitScore(gameMode, data.user, score, accuracy)
    }

    delete games[account_id]

    res.json({
        score,
        correctDate: stringToLvlDate(correctDate)
    })
})

router.post("/endGame", async (req, res) => {
    const token = req.headers.authorization || ""
    const data = await checkToken(token)
    
    if (!data.user) {
        res.status(401).send(`Invalid token: ${data.error}`)
        return
    }

    const account_id = data.user.account_id

    const gameExists = Object.keys(games).includes(String(account_id))
    if (gameExists) {
        submitScore(games[account_id].options.mode, data.user, 0, 0)
    }

    if (!Object.keys(games).includes(account_id.toString())) {
        res.status(404).send("game does not exist")
        return
    }

    delete games[account_id]

    res.status(200).send()
})

router.get("/account/:id", async (req, res) => {
    const response = await getUser(req.params.id)

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
        WHERE total_score > 2500
        ORDER BY (total_score * total_score) * 1.0 / max_score DESC
        LIMIT 100
    `)
    res.json(results)
})

const port = process.env.PORT || 8000

console.log(`listening on port ${port}`)
httpServer.listen(port)
