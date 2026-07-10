CREATE TABLE users (id INT, name TEXT, age INT, is_active BOOLEAN);
INSERT INTO users (id, name, age, is_active) VALUES (1, 'Alice', 25, true);
INSERT INTO users (id, name, age, is_active) VALUES (2, 'Bob', 30, false);
SELECT id, name, age FROM users WHERE age >= 25;
UPDATE users SET age = 31 WHERE name = 'Bob';
DELETE FROM users WHERE id = 1;
