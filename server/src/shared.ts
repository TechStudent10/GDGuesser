import "dotenv/config"

import * as fs from "node:fs"
import * as path from "node:path"
import * as jwt from "jsonwebtoken"

import sqlite3 from "sqlite3"
import { open } from "sqlite"

export const lvlStats = fs.readFileSync(path.join(__dirname, "..", "resources", "LevelStats.dat")).toString()
export const unsortedLevelIds = lvlStats.split(",").filter((_, index) => index % 2 === 0).map(val => parseInt(val));

// taken from https://github.com/Alphalaneous/Random-Tab/blob/main/src/RandomLayer.hpp#L27
export const ID_CUTOFFS: {[key: string]: number[]} = {
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
    "2.2": [97454812, 200000000]
}

export const VERSION_WEIGHTS: {[key: string]: number[]} = {
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

export const SECRET_KEY = process.env.SECRET_KEY || ""

export const levelIds = (() => {
    const result: {[key: string]: number[]} = {}
    const start = Date.now()
    Object.keys(ID_CUTOFFS).forEach(update => {
        result[update] = unsortedLevelIds.filter((id) => id >= ID_CUTOFFS[update][0] && id <= ID_CUTOFFS[update][1])
    })
    console.log(`id cutoffs took ${Date.now() - start}ms`)
    return result
})()

export const ROBTOP_LEVELS = [
    97459478,
    97459514,
    97459509,
    97459505,
    97459496,
    77243258,
    77243257,
    77243256,
    77243245,
    77242482,
    77242480,
    77242477,
    77242475,
    77242474,
    77241653,
    77241652,
    77241651,
    77241647,
    77241646,
    77240642,
    77240637,
    77240636,
    77240633,
    77240630,
    77239604,
    77239599,
    77239595,
    77239588,
    77239587,
    77238488,
    77238483,
    77238477,
    77238474,
    77238470,
    77237548,
    77237547,
    77237545,
    77237542,
    77237540,
    77236593,
    77236592,
    77236588,
    77236587,
    116776010,
    64760396,
    38864397,
    37731209,
    29587046,
    28359567
]

export async function openDB() {
    return await open({
        filename: process.env.DATABASE_FILE || "./database.db",
        driver: sqlite3.Database
    })
}

export type UserToken = {
    username: string
    account_id: number
}

export type User = {
    account_id: number
    username: string
    total_score: number
    icon_id: number
    color1: number
    color2: number
    color3: number
    accuracy: number
    max_score: number
    total_normal_guesses: number
    total_hardcore_guesses: number
    leaderboard_position: number
}

export type LevelDate = {
    year: number
    month: number
    day: number
}

export enum GameMode {
    Normal,
    Hardcore
}

export type GameOptions = {
    mode: GameMode,
    versions: string[]
}

export type Game = {
    accountId: number
    currentLevelId: number
    options: GameOptions
}

// The Game (Multiplayer Edition)
export type GameMP = {
    // participant account IDs
    players: {[key: string]: User}

    currentLevelId: number
    options: GameOptions

    joinCode: string
    scores: {[key: string]: number}

    playersReady: number[]
}

export function stringToLvlDate(str: string): LevelDate {
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

export function calcScore(guess: LevelDate, correct: LevelDate, options: GameOptions): number[] {
    const limit = options.mode === GameMode.Normal ? 500 : 600;

    const guessDate = new Date(guess.year, guess.month - 1, guess.day)
    const correctDate = new Date(correct.year, correct.month - 1, correct.day)

    const diffDays = Math.ceil(Math.abs(guessDate.getTime() - correctDate.getTime()) / (1000 * 3600 * 24))
    const score = diffDays <= 7 ? limit : Math.max(limit - (diffDays - 7), 0)
    const accuracy = score / limit * 100

    return [score, accuracy]
}

export function getRandomElement<T>(arr: Array<T>) {
    return arr[Math.floor((Math.random() * arr.length) % arr.length)]
}

export function getRandomLevelId(allowedVersions: string[], weightList: number) {
    // TODO: remove after april fools
    if (Math.random() >= 0.5) {
        return getRandomElement(ROBTOP_LEVELS)
    }
    let versions = Object.keys(ID_CUTOFFS)
    if (allowedVersions.length > 0) {
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

export async function checkToken(token: string): Promise<{
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

export async function getUserBy(sortingMechanism: string, sortObject: any): Promise<User | undefined> {
    const db = await openDB()

    const response = await db.get(`
        SELECT * FROM (
            SELECT *, ROW_NUMBER() OVER (
                ORDER BY (total_score * total_score) * 1.0 / max_score DESC
            ) AS leaderboard_position FROM scores
        ) ranked WHERE ${sortingMechanism} = ? COLLATE NOCASE;
    `, sortObject)

    if (!response) {
        return undefined
    }

    return response
}

export async function getUserByID(account_id: string | number) {
    return await getUserBy("account_id", account_id)
}

export async function getUserByName(username: string) {
    return await getUserBy("username", username)
}

// stole this from Creation Rotation :fire:
// https://stackoverflow.com/a/7228322
export function generateCode() {
    return Math.floor(Math.random() * (999_999) + 1).toString().padStart(6, "0")
    // return "000001" // this is so that i dont suffer
}
