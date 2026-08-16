import * as path from 'path';
import * as vs from 'vscode';
import { Prefs } from '../config/prefs';
import * as vsUtils from './vscodeUtils';
import {isWin32} from './utils';

export function substituteRemotePath(localFilePath: string): string {
    if (!Prefs.isRemoteScenario()) {
        return localFilePath;
    }
    const remoteProjectDirPath = Prefs.getRemoteRoot();
    const projectDirName = path.basename(vsUtils.getProjectDirByOpenedFile().fsPath);
    let remoteFilePath = fsJoin(remoteProjectDirPath,
        localFilePath.slice(localFilePath.indexOf(projectDirName) + projectDirName.length));
    if (isWin32()) {
        remoteFilePath = remoteFilePath.replace(/\\/g, '/');
    }
    remoteFilePath = normalizeRawPosixPath(remoteFilePath);
    return remoteFilePath;
}

export function substituteLocalPath(remoteFilePath: string): string {
    if (!Prefs.isRemoteScenario()) {
        return remoteFilePath;
    }
    const root = vsUtils.getProjectDirByOpenedFile();
    const remoteRoot = Prefs.getRemoteRoot();
    const projectDirName = path.basename(remoteRoot);
    const relative = remoteFilePath.slice(remoteFilePath.indexOf(projectDirName) + projectDirName.length);
    const localFilePath = fsJoin(root.fsPath, relative);
    if (isWin32()) {
        return path.win32.normalize(localFilePath);
    }
    return localFilePath;
}

/**
 * Bring a path to a form two spellings of the same location agree on.
 *
 * On Windows that is three things at once: either slash is a separator, the
 * filesystem does not distinguish case, and the drive letter gets written both
 * ways. VS Code hands out `d:\Dev\proj` while a setting is typed `D:/Dev/proj`,
 * and neither is more correct than the other.
 */
export function normalizeForComparison(anyPath: string): string {
    let normalized = path.posix.normalize(anyPath.replace(/\\/g, '/')).replace(/\/+$/, '');
    if (isWin32()) {
        normalized = normalized.toLowerCase();
    }
    return normalized;
}

/** Whether two paths name the same place, however each was spelled. */
export function samePath(left: string, right: string): boolean {
    return normalizeForComparison(left) === normalizeForComparison(right);
}

export function normalizeRawPosixPath(posixPath: string): string {
    posixPath = path.posix.normalize(posixPath);
    posixPath = posixPath.replace(/\/+$/, '');
    return posixPath;
}

export function getRealFilePath(editorPath: string, optionalClickedPath: vs.Uri | undefined): string {
    if (optionalClickedPath) {
        return optionalClickedPath?.fsPath;
    }
    return editorPath;
}

export function getRootPath(): string | undefined {
    return vs.workspace.workspaceFolders?.[0].uri.fsPath;
}

export function fsJoin(...paths: string[]): string {
    if (isWin32()) {
        return path.win32.join(...paths);
    } else {
        return path.posix.join(...paths);
    }
}
