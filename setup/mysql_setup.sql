-- sudo mysql -u root -p < mysql_setup.sql
-- prompt for root password : <puth the root password for server>



-- =============================================================
--           MySQL Setup Script for Key-Value Store
-- =============================================================


-- Create the database if it doesn't exist
CREATE DATABASE IF NOT EXISTS key_value_DB_744;

-- Create a dedicated user 'kvuser' with password 'kvpass'
CREATE USER IF NOT EXISTS 'user_744'@'localhost' IDENTIFIED BY 'key_744';

-- Grant all privileges on the key_value_DB_744 database to this user
GRANT ALL PRIVILEGES ON key_value_DB_744.* TO 'user_744'@'localhost';

-- Apply privilege changes immediately
FLUSH PRIVILEGES;

-- Use the key_value_DB_744 database
USE key_value_DB_744;

-- Create the table structure used by the C++ server
CREATE TABLE IF NOT EXISTS key_value_table (k VARCHAR(255) PRIMARY KEY, v TEXT );

-- Done
SELECT 'key_value_DB_744 setup complete' AS status;
