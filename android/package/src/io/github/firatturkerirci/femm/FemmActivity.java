// SPDX-License-Identifier: MIT
// Copyright (C) 2026 David Meeker and contributors
//
// FemmActivity - Qt's activity plus "Share to FEMM": text or a .py file shared
// from another app (an AI assistant, a browser, a file manager) is kept here
// until the C++ side collects it with takeSharedText() and puts it into the
// script panel.

package io.github.firatturkerirci.femm;

import android.content.Intent;
import android.net.Uri;
import android.os.Bundle;

import java.io.ByteArrayOutputStream;
import java.io.InputStream;
import java.nio.charset.StandardCharsets;

import org.qtproject.qt.android.bindings.QtActivity;

public class FemmActivity extends QtActivity
{
    private static String s_shared = null;
    private static final int MAX_BYTES = 4 * 1024 * 1024;

    @Override
    public void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        take(getIntent());
    }

    @Override
    protected void onNewIntent(Intent intent)
    {
        super.onNewIntent(intent);
        setIntent(intent);
        take(intent);
    }

    public static synchronized String takeSharedText()
    {
        String t = s_shared;
        s_shared = null;
        return t;
    }

    private static synchronized void put(String t)
    {
        if (t != null && !t.trim().isEmpty())
            s_shared = t;
    }

    private void take(Intent intent)
    {
        if (intent == null) return;
        try {
            String action = intent.getAction();
            if (Intent.ACTION_SEND.equals(action)) {
                String text = intent.getStringExtra(Intent.EXTRA_TEXT);
                if (text == null) {
                    Uri uri = intent.getParcelableExtra(Intent.EXTRA_STREAM);
                    if (uri != null) text = read(uri);
                }
                put(text);
            } else if (Intent.ACTION_VIEW.equals(action) && intent.getData() != null) {
                put(read(intent.getData()));
            }
        } catch (Exception e) {
            // A share we cannot read is simply ignored.
        }
    }

    private String read(Uri uri) throws Exception
    {
        try (InputStream in = getContentResolver().openInputStream(uri)) {
            if (in == null) return null;
            ByteArrayOutputStream out = new ByteArrayOutputStream();
            byte[] buf = new byte[8192];
            int n, total = 0;
            while ((n = in.read(buf)) > 0) {
                total += n;
                if (total > MAX_BYTES) return null;
                out.write(buf, 0, n);
            }
            return new String(out.toByteArray(), StandardCharsets.UTF_8);
        }
    }
}
