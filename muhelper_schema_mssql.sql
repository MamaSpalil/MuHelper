-- ============================================================
--  MuHelper v2 — Database Schema (MSSQL 2008 R2)
-- ============================================================

-- Main config (96 bytes blob)
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[MuHelperConfig]') AND type = N'U')
BEGIN
    CREATE TABLE [dbo].[MuHelperConfig] (
        [CharIdx]   INT           NOT NULL,
        [cfg]       VARBINARY(96) NOT NULL,
        [UpdatedAt] DATETIME      NOT NULL DEFAULT GETDATE(),
        CONSTRAINT [PK_MuHelperConfig] PRIMARY KEY CLUSTERED ([CharIdx])
    );
END
GO

-- Named profiles (up to 5 per character)
IF NOT EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[MuHelperProfiles]') AND type = N'U')
BEGIN
    CREATE TABLE [dbo].[MuHelperProfiles] (
        [CharIdx]   INT           NOT NULL,
        [SlotIdx]   TINYINT       NOT NULL,
        [Name]      VARCHAR(16)   NOT NULL DEFAULT '',
        [cfg]       VARBINARY(96) NOT NULL,
        [UpdatedAt] DATETIME      NOT NULL DEFAULT GETDATE(),
        CONSTRAINT [PK_MuHelperProfiles] PRIMARY KEY CLUSTERED ([CharIdx], [SlotIdx])
    );
END
GO

-- ============================================================
--  Stored Procedures for MuHelper operations
-- ============================================================

-- Load config for a character
IF EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[MuHelper_LoadConfig]') AND type = N'P')
    DROP PROCEDURE [dbo].[MuHelper_LoadConfig]
GO
CREATE PROCEDURE [dbo].[MuHelper_LoadConfig]
    @CharIdx INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT [cfg] FROM [dbo].[MuHelperConfig] WHERE [CharIdx] = @CharIdx;
END
GO

-- Save config for a character (insert or update)
IF EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[MuHelper_SaveConfig]') AND type = N'P')
    DROP PROCEDURE [dbo].[MuHelper_SaveConfig]
GO
CREATE PROCEDURE [dbo].[MuHelper_SaveConfig]
    @CharIdx INT,
    @cfg     VARBINARY(96)
AS
BEGIN
    SET NOCOUNT ON;
    IF EXISTS (SELECT 1 FROM [dbo].[MuHelperConfig] WHERE [CharIdx] = @CharIdx)
        UPDATE [dbo].[MuHelperConfig]
           SET [cfg] = @cfg, [UpdatedAt] = GETDATE()
         WHERE [CharIdx] = @CharIdx;
    ELSE
        INSERT INTO [dbo].[MuHelperConfig] ([CharIdx], [cfg])
        VALUES (@CharIdx, @cfg);
END
GO

-- Load all profiles for a character
IF EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[MuHelper_LoadProfiles]') AND type = N'P')
    DROP PROCEDURE [dbo].[MuHelper_LoadProfiles]
GO
CREATE PROCEDURE [dbo].[MuHelper_LoadProfiles]
    @CharIdx INT
AS
BEGIN
    SET NOCOUNT ON;
    SELECT [SlotIdx], [Name], [cfg]
      FROM [dbo].[MuHelperProfiles]
     WHERE [CharIdx] = @CharIdx
     ORDER BY [SlotIdx];
END
GO

-- Save a profile (insert or update)
IF EXISTS (SELECT * FROM sys.objects WHERE object_id = OBJECT_ID(N'[dbo].[MuHelper_SaveProfile]') AND type = N'P')
    DROP PROCEDURE [dbo].[MuHelper_SaveProfile]
GO
CREATE PROCEDURE [dbo].[MuHelper_SaveProfile]
    @CharIdx INT,
    @SlotIdx TINYINT,
    @Name    VARCHAR(16),
    @cfg     VARBINARY(96)
AS
BEGIN
    SET NOCOUNT ON;
    IF EXISTS (SELECT 1 FROM [dbo].[MuHelperProfiles]
               WHERE [CharIdx] = @CharIdx AND [SlotIdx] = @SlotIdx)
        UPDATE [dbo].[MuHelperProfiles]
           SET [Name] = @Name, [cfg] = @cfg, [UpdatedAt] = GETDATE()
         WHERE [CharIdx] = @CharIdx AND [SlotIdx] = @SlotIdx;
    ELSE
        INSERT INTO [dbo].[MuHelperProfiles] ([CharIdx], [SlotIdx], [Name], [cfg])
        VALUES (@CharIdx, @SlotIdx, @Name, @cfg);
END
GO

-- Migrate from v1 (if upgrading):
-- The cfg blob grew from 60 bytes to 96 bytes.
-- Old rows will be zero-padded (safe defaults).
-- ALTER TABLE [dbo].[MuHelperConfig] ALTER COLUMN [cfg] VARBINARY(96) NOT NULL;
