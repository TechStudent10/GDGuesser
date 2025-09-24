// events the server can send to the client

import { GameMP, GameOptions, LevelDate } from "../shared"

export type DuelCreated = {
    joinCode: string
}

export type DuelJoined = {
    status: number
}

export type DuelUpdated = {
    duel: GameMP
}

export type RoundStarted = {
    options: GameOptions
    levelId: number
}

export type DuelResults = {
    scores: {[key: string]: number}
    totalScores: {[key: string]: number}
    guessedDates: {[key: string]: LevelDate}
    correctDate: LevelDate
}

export type ReceiveDuel = {
    duel: GameMP
}

export type OpponentGuessed = {}

export type DuelEnded = {
    duel: GameMP
    // account IDs (GameMP alr includes the users)
    winner: number
    loser: number
}
