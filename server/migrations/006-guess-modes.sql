-- Up

ALTER TABLE guesses ADD COLUMN mode INT;

-- Down

ALTER TABLE guesses DROP COLUMN mode;
