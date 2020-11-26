/**
 * @file heartbeats.h
 * @brief Classes for heartbeating Backup configuration and status
 *
 * (c) 2020 by Mega Limited, Auckland, New Zealand
 *
 * This file is part of the MEGA SDK - Client Access Engine.
 *
 * Applications using the MEGA API must present a valid application key
 * and comply with the the rules set forth in the Terms of Service.
 *
 * The MEGA SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @copyright Simplified (2-clause) BSD License.
 *
 * You should have received a copy of the license along with this
 * program.
 */

#pragma once

#include "types.h"
#include <memory>
#include <functional>
#include "mega/command.h"

namespace mega
{

struct UnifiedSync;

/**
 * @brief The HeartBeatBackupInfo class
 * This class holds the information that will be heartbeated
 */

class HeartBeatBackupInfo : public CommandListener
{
public:
    HeartBeatBackupInfo();
    HeartBeatBackupInfo(HeartBeatBackupInfo&&) = default;
    HeartBeatBackupInfo& operator=(HeartBeatBackupInfo&&) = default;
    virtual ~HeartBeatBackupInfo() = default;

    MEGA_DISABLE_COPY(HeartBeatBackupInfo)

    virtual int status() const;

    virtual double progress() const;
    virtual void invalidateProgress();

    virtual m_time_t lastAction() const;

    virtual handle lastItemUpdated() const;

    virtual Command *runningCommand() const;
    virtual void setRunningCommand(Command *runningCommand);

    virtual void onCommandToBeDeleted(Command *command) override;
    virtual m_time_t lastBeat() const;
    virtual void setLastBeat(const m_time_t &lastBeat);
    virtual void setLastAction(const m_time_t &lastAction);
    virtual void setStatus(const int &status);
    virtual void setProgress(const double &progress);
    virtual void setLastSyncedItem(const handle &lastItemUpdated);

    virtual void updateStatus(UnifiedSync& us) {}

    int32_t mPendingUps = 0;
    int32_t mPendingDowns = 0;

protected:
    int mStatus = 0;
    double mProgress = 0;
    bool mProgressInvalid = true;

    handle mLastItemUpdated = UNDEF; // handle of node most recently updated

    m_time_t mLastAction = -1;   //timestamps of the last action
    m_time_t mLastBeat = -1;     //timestamps of the last beat

    Command *mRunningCommand = nullptr;

    void updateLastActionTime();
};

/**
 * @brief Backup info controlled by transfers
 * progress is advanced with transfer progress
 * by holding the count of total and transferred bytes
 *
 */
class HeartBeatTransferProgressedInfo : public HeartBeatBackupInfo
{
public:
    double progress() const override;

    long long mTotalBytes = 0;
    long long mTransferredBytes = 0;

    void adjustTransferCounts(int32_t upcount, int32_t downcount, long long totalBytes, long long transferBytes);

};

#ifdef ENABLE_SYNC
class HeartBeatSyncInfo : public HeartBeatTransferProgressedInfo
{
public:
    enum Status
    {
        UPTODATE = 1, // Up to date: local and remote paths are in sync
        SYNCING = 2, // The sync engine is working, transfers are in progress
        PENDING = 3, // The sync engine is working, e.g: scanning local folders
        INACTIVE = 4, // Sync is not active. A state != ACTIVE should have been sent through '''sp'''
        UNKNOWN = 5, // Unknown status
    };

    HeartBeatSyncInfo();
    MEGA_DISABLE_COPY(HeartBeatSyncInfo)

    virtual void updateStatus(UnifiedSync& us) override;
private:

};
#endif

/**
 * @brief Information for registration/update of a backup
 */
class MegaBackupInfo
{
public:
    MegaBackupInfo(BackupType type, string localFolder, handle megaHandle, int state, int substate, string extra);

    BackupType type() const;

    string localFolder() const;

    handle megaHandle() const;

    int state() const;

    int subState() const;

    string extra() const;

protected:
    BackupType mType;
    string mLocalFolder;
    handle mMegaHandle;
    int mState;
    int mSubState;
    string mExtra;
};

#ifdef ENABLE_SYNC
class MegaBackupInfoSync : public MegaBackupInfo
{
public:
    enum State
    {
        ACTIVE = 1,             // Working fine (enabled)
        FAILED = 2,             // Failed (permanently disabled)
        TEMPORARY_DISABLED = 3, // Temporarily disabled due to a transient situation (e.g: account blocked). Will be resumed when the condition passes
        DISABLED = 4,           // Disabled by the user
        PAUSE_UP = 5,           // Active but upload transfers paused in the SDK
        PAUSE_DOWN = 6,         // Active but download transfers paused in the SDK
        PAUSE_FULL = 7,         // Active but transfers paused in the SDK
    };
    MegaBackupInfoSync(UnifiedSync&);

    void updatePauseState(MegaClient *client);

    static BackupType getSyncType(const SyncConfig& config);
    static int getSyncState (UnifiedSync&);
    static int getSyncSubstatus (UnifiedSync&);
    string getSyncExtraData(UnifiedSync&);

    bool operator==(const MegaBackupInfoSync& o) const;

private:
    static int calculatePauseActiveState(MegaClient *client);
};
#endif

class MegaBackupMonitor
{
public:
    explicit MegaBackupMonitor(MegaClient * client);

    void beat(); // produce heartbeats!

    void onSyncConfigChanged();
    void updateOrRegisterSync(UnifiedSync&);

private:
    void digestPutResult(handle backupId, UnifiedSync* syncPtr);

    static constexpr int MAX_HEARBEAT_SECS_DELAY = 60*30; // max time to wait before a heartbeat for unchanged backup

    mega::MegaClient *mClient = nullptr;

    void updateBackupInfo(handle backupId, const MegaBackupInfo &info);
    void registerBackupInfo(const MegaBackupInfo &info, UnifiedSync* syncPtr);

    void beatBackupInfo(UnifiedSync& us);
    void calculateStatus(HeartBeatBackupInfo *hbs, UnifiedSync& us);
};
}
