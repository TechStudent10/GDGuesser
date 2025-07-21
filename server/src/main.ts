import "dotenv/config"

import express from "express"
import { createServer } from "http"

import * as jwt from "jsonwebtoken"

import { version as serverVersion } from "../package.json"
import setupMP from "./multiplayer/mp"
import {
    openDB,
    Game,
    GameMode,
    UserToken,
    SECRET_KEY,
    getUserByID,
    checkToken,
    ID_CUTOFFS,
    GameOptions,
    getRandomLevelId,
    calcScore,
    stringToLvlDate,
    getUserByName
} from "./shared"

const router = express()
const httpServer = createServer(router)
router.use(express.json())

setupMP(httpServer);

(async () => {
    const db = await openDB();
    db.migrate()
})()

const games: {[key: string]: Game} = {}

async function submitScore(
    mode: GameMode,
    user: UserToken,
    guess_history: boolean,
    accuracy: number,
    score?: number,
    correct_date?: string,
    guessed_date?: string,
    level_id?: number,
    level_name?: string,
    level_creator?: string,
) {
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

    if (guess_history) {
        await db.run(`
            INSERT INTO guesses (
                account_id,
                level_id,
                score,
                mode,
                correct_date,
                guessed_date,
                level_name,
                level_creator
            ) VALUES (?, ?, ?, ?, ?, ?, ?, ?)
        `, user.account_id, level_id, score, mode, correct_date, guessed_date, level_name || "-", level_creator || "-")
    }
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

router.use((req, res, next) => {
    if (req.method.toLowerCase() != "post") {
        next()
        return
    }

    const version = req.headers["version"] as string | undefined
    if (!version) {
        res.status(400).send("Valid version required")
        return
    }

    if (version !== serverVersion) {
        res.status(400).send(`Server version ${serverVersion} and client version ${version} mismatch. Please either update the GDGuesser mod or contact the owner of this server to update to the latest version.`)
        return
    }

    // all checks pass; we good!
    next()
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
        account_id,
        username: data["username"]
    }
    const jwtToken = jwt.sign(tokenInfo, SECRET_KEY)

    res.status(200).json({
        user: await getUserByID(account_id),
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

    if (gameExists && games[account_id].options.versions.length === Object.keys(ID_CUTOFFS).length) {
        submitScore(games[account_id].options.mode, data.user, false, 0, 0)
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

    let correctDate: string;
    
    try {
        const res = await fetch(`https://history.geometrydash.eu/api/v1/date/level/${levelId}/`)
        if (!res.ok) throw new Error("History server returned non-OK status")

        const json = await res.json()
        correctDate = json["approx"]["estimation"].split("T")[0]
    } catch (err) {
        res.status(522).send("GDHistory connection timed out")
        return
    }

    let levelInfo;
    try {
        const res = await fetch(`https://history.geometrydash.eu/api/v1/level/${levelId}`)
        if (!res.ok) throw new Error("History server returned non-OK status")

        const json = await res.json()
        levelInfo = json
    } catch (err) {
        res.status(522).send("GDHistory connection timed out")
        return
    }

    const scoreResult = calcScore(stringToLvlDate(date), stringToLvlDate(correctDate), games[account_id].options)
    const score = scoreResult[0]
    const accuracy = scoreResult[1]
    
    const gameMode = games[account_id].options.mode
    
    // only submit if all versions are selected
    if (games[account_id].options.versions.length === Object.keys(ID_CUTOFFS).length) {
        await submitScore(gameMode, data.user, true, accuracy, score, correctDate, date, levelId, levelInfo["cache_level_name"], levelInfo["cache_username"])
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
    if (gameExists && games[account_id].options.versions.length === Object.keys(ID_CUTOFFS).length) {
        submitScore(games[account_id].options.mode, data.user, false, 0, 0)
    }

    if (!Object.keys(games).includes(account_id.toString())) {
        res.status(404).send("game does not exist")
        return
    }

    delete games[account_id]

    res.status(200).send()
})

router.get("/account/:id", async (req, res) => {
    const response = await getUserByID(req.params.id)

    if (!response) {
        res.status(404).send("no account found")
        return
    }

    res.json(
        response
    )
})

router.get("/accountByName/:username", async (req, res) => {
    const response = await getUserByName(req.params.username)

    if (!response) {
        res.status(404).send("no account found")
        return
    }

    res.json(
        response
    )
})

router.get("/guesses/:account_id", async (req, res) => {
    const page = parseInt(req.query.page?.toString() || "0")
    const per_page = 10

    const db = await openDB()
    const results = await db.all(`
        SELECT * FROM guesses WHERE account_id = ? ORDER BY id DESC LIMIT ${per_page} OFFSET ${page * per_page}
    `, req.params.account_id)

    const total_pages = (await db.get("SELECT COUNT(*) AS pages FROM guesses WHERE account_id = ?", req.params.account_id))["pages"] / per_page

    res.json({
        guesses: results,
        page,
        total_pages: total_pages === 0 ? 1 : Math.ceil(total_pages)
    })
})

router.get("/leaderboard", async (req, res) => {
    const db = await openDB()
    const results = await db.all(`
        SELECT * FROM (
            SELECT *, ROW_NUMBER() OVER (
                ORDER BY (total_score * total_score) * 1.0 / max_score DESC
            ) AS leaderboard_position FROM scores
        ) ranked
        WHERE total_score >= 2500
        ORDER BY (total_score * total_score) * 1.0 / max_score DESC
        LIMIT 100
    `)
    res.json(results)
})

const port = process.env.PORT || 8000

console.log(`listening on port ${port}`)
httpServer.listen(port)
