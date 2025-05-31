-- Up

CREATE TABLE guesses (
    id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL,
    account_id INTEGER NOT NULL,
    level_id INTEGER NOT NULL,
    score INTEGER NOT NULL,
    correct_date TEXT NOT NULL,
    guessed_date TEXT NOT NULL,
    level_name TEXT NOT NULL,
    level_creator TEXT NOT NULL
);

-- Down

DROP TABLE guesses;
