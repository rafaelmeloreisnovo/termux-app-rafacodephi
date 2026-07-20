package com.termux.rafacodephi.loader;

import android.content.ContentProvider;
import android.content.ContentValues;
import android.database.Cursor;
import android.database.MatrixCursor;
import android.net.Uri;
import android.os.ParcelFileDescriptor;
import android.provider.OpenableColumns;

import java.io.File;
import java.io.FileNotFoundException;
import java.io.IOException;
import java.util.regex.Pattern;

/** Read-only URI handoff for one SHA-pinned bootstrap ZIP in loader-private storage. */
public final class VerifiedBootstrapProvider extends ContentProvider {

    private static final Pattern SAFE_NAME = Pattern.compile(
            "^bootstrap-(aarch64|arm|i686|x86_64)-[0-9a-f]{64}\\.zip$");

    @Override
    public boolean onCreate() {
        return true;
    }

    @Override
    public Cursor query(Uri uri, String[] projection, String selection,
                        String[] selectionArgs, String sortOrder) {
        File file = resolve(uri);
        String[] columns = projection == null
                ? new String[]{OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE}
                : projection;
        MatrixCursor cursor = new MatrixCursor(columns, 1);
        Object[] row = new Object[columns.length];
        for (int i = 0; i < columns.length; i++) {
            if (OpenableColumns.DISPLAY_NAME.equals(columns[i])) {
                row[i] = file.getName();
            } else if (OpenableColumns.SIZE.equals(columns[i])) {
                row[i] = file.length();
            } else {
                row[i] = null;
            }
        }
        cursor.addRow(row);
        return cursor;
    }

    @Override
    public String getType(Uri uri) {
        resolve(uri);
        return "application/zip";
    }

    @Override
    public ParcelFileDescriptor openFile(Uri uri, String mode) throws FileNotFoundException {
        if (!"r".equals(mode)) throw new FileNotFoundException("READ_ONLY_PROVIDER");
        File file = resolve(uri);
        if (!file.isFile() || !file.canRead()) throw new FileNotFoundException("FILE_NOT_READY");
        return ParcelFileDescriptor.open(file, ParcelFileDescriptor.MODE_READ_ONLY);
    }

    @Override
    public Uri insert(Uri uri, ContentValues values) {
        throw new UnsupportedOperationException("READ_ONLY_PROVIDER");
    }

    @Override
    public int delete(Uri uri, String selection, String[] selectionArgs) {
        throw new UnsupportedOperationException("READ_ONLY_PROVIDER");
    }

    @Override
    public int update(Uri uri, ContentValues values, String selection,
                      String[] selectionArgs) {
        throw new UnsupportedOperationException("READ_ONLY_PROVIDER");
    }

    private File resolve(Uri uri) {
        if (getContext() == null) throw new IllegalStateException("CONTEXT_UNAVAILABLE");
        if (!"content".equals(uri.getScheme())
                || !BootstrapInstallContract.PROVIDER_AUTHORITY.equals(uri.getAuthority())) {
            throw new IllegalArgumentException("INVALID_BOOTSTRAP_URI");
        }
        String name = uri.getLastPathSegment();
        if (name == null || !SAFE_NAME.matcher(name).matches()) {
            throw new IllegalArgumentException("INVALID_BOOTSTRAP_NAME");
        }
        File root = new File(getContext().getFilesDir(), "verified");
        File candidate = new File(root, name);
        try {
            String canonicalRoot = root.getCanonicalPath() + File.separator;
            String canonicalCandidate = candidate.getCanonicalPath();
            if (!canonicalCandidate.startsWith(canonicalRoot)) {
                throw new IllegalArgumentException("PATH_OUTSIDE_VERIFIED_ROOT");
            }
        } catch (IOException e) {
            throw new IllegalArgumentException("CANONICAL_PATH_FAILED", e);
        }
        return candidate;
    }
}
