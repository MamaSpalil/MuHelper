-- ============================================================
--  MuHelper v2 — Database Schema (MySQL — DEPRECATED)
--  NOTE: The production database uses MSSQL 2008 R2.
--  See muhelper_schema_mssql.sql for the active schema.
--  This file is kept for reference only.
-- ============================================================

-- Main config (96 bytes blob)
CREATE TABLE IF NOT EXISTS `MuHelperConfig` (
    `CharIdx`   INT UNSIGNED NOT NULL,
    `cfg`       BLOB         NOT NULL,
    `UpdatedAt` TIMESTAMP    NOT NULL DEFAULT CURRENT_TIMESTAMP
                             ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`CharIdx`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Named profiles (up to 5 per character)
CREATE TABLE IF NOT EXISTS `MuHelperProfiles` (
    `CharIdx`   INT UNSIGNED    NOT NULL,
    `SlotIdx`   TINYINT UNSIGNED NOT NULL,
    `Name`      VARCHAR(16)     NOT NULL DEFAULT '',
    `cfg`       BLOB            NOT NULL,
    `UpdatedAt` TIMESTAMP       NOT NULL DEFAULT CURRENT_TIMESTAMP
                                ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (`CharIdx`, `SlotIdx`)
) ENGINE=InnoDB DEFAULT CHARSET=latin1;

-- Migrate from v1 (if upgrading):
-- The cfg blob grew from 60 bytes to 96 bytes.
-- Old rows will auto-pad with zeros (safe defaults).
-- ALTER TABLE MuHelperConfig MODIFY cfg BLOB NOT NULL;
