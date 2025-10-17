/*
 * This file is part of Veretino,
 * licensed under the GNU GPLv3.
 * https://github.com/artemvlas/veretino
*/
#ifndef FILEVALUES_H
#define FILEVALUES_H

#include <QObject>

struct FileValues {
    Q_GADGET

public:
    enum HashingPurpose : quint8 {
        Generic,           // the checksum has been calculated and is ready for further processing (copy or save)
        AddToDb,
        Verify,
        CopyToClipboard,   // the calculated checksum is intended to be copied to the clipboard
        SaveToDigestFile   // ... stored in a summary file
    }; // enum HashingPurpose

    enum FileStatus {
        NotSet = 0,

        Queued = 1 << 0,        // added to the processing queue
        Calculating = 1 << 1,   // checksum is being calculated
        Verifying = 1 << 2,     // checksum is being verified
        CombProcessing = Queued | Calculating | Verifying,

        NotChecked = 1 << 3,    // available for verification
        NotCheckedMod = 1 << 4, // same, but the file modif. time was checked and it is later than the db creation
        Matched = 1 << 5,       // checked, checksum matched
        Mismatched = 1 << 6,    // checked, checksum did not match
        New = 1 << 7,           // a file that is present on disk but not in the database
        Missing = 1 << 8,       // not on disk, but present in the database
        Added = 1 << 9,         // item (file path and its checksum) has been added to the database
        Removed = 1 << 10,      // item^ removed from the database
        Updated = 1 << 11,      // the checksum has been updated
        Imported = 1 << 12,     // the checksum was imported from another file
        Moved = 1 << 13,        // the newly calculated checksum corresponds to some missing item (renamed or moved file)
        MovedOut = 1 << 14,     // former Missing when moving
        UnPermitted = 1 << 15,  // no read permissions
        ReadError = 1 << 16,    // an error occurred during reading

        CombNotChecked = NotChecked | NotCheckedMod | Imported,
        CombAvailable = CombNotChecked | Matched | Mismatched | Added | Updated | Moved,
        CombHasChecksum = CombAvailable | Missing,
        CombUpdatable = New | Missing | Mismatched,
        CombDbChanged = Added | Removed | Updated | Imported | Moved,
        CombChecked = Matched | Mismatched,
        CombMatched = Matched | Added | Updated | Moved,
        CombNewLost = New | Missing,
        CombUnreadable = UnPermitted | ReadError,
        CombNoFile = Missing | Removed,
        CombCalcError = CombNoFile | CombUnreadable
    }; // enum FileStatus

    Q_ENUM(FileStatus)
    Q_DECLARE_FLAGS(FileStatuses, FileStatus)

    /*** Constructors ***/
    FileValues(FileStatus fileStatus = FileStatus::NotSet, qint64 fileSize = -1)
        : status(fileStatus), size(fileSize) {}

    FileValues(HashingPurpose hashPurpose, qint64 fileSize = -1)
        : hash_purpose(hashPurpose), size(fileSize) {}

    FileValues(qint64 fileSize)
        : size(fileSize) {}

    /*** Funcs ***/
    QString& defaultChecksum() { return (hash_purpose == Verify) ? reChecksum : checksum; }
    const QString& defaultChecksum() const { return (hash_purpose == Verify) ? reChecksum : checksum; }
    explicit operator bool() const { return !defaultChecksum().isEmpty(); }

    /*** Variables ***/
    FileStatus status = FileStatus::NotSet;
    HashingPurpose hash_purpose = Generic;

    qint64 hash_time = -1;    // hashing time in milliseconds, -1 if not set
    qint64 size = -1;         // file size in bytes, -1 if not set
    QString checksum;         // newly computed or imported from the database
    QString reChecksum;       // the re-computed one (for verification purpose)
}; // struct FileValues

using FileStatus = FileValues::FileStatus;
using FileStatuses = FileValues::FileStatuses;
Q_DECLARE_OPERATORS_FOR_FLAGS(FileValues::FileStatuses)

// {relative path to file : FileValues struct}
using FileList = QMap<QString, FileValues>;

#endif // FILEVALUES_H
