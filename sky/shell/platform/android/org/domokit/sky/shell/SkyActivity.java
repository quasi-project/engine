// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.domokit.sky.shell;

import android.app.Activity;
import android.content.Intent;
import android.os.Build;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.Window;
import android.view.WindowManager;

import io.flutter.view.FlutterMain;
import io.flutter.view.FlutterView;

import org.chromium.base.PathUtils;
import org.chromium.base.TraceEvent;
import org.chromium.mojom.sky.EventType;
import org.chromium.mojom.sky.InputEvent;

import java.io.File;
import java.util.ArrayList;


/**
 * Base class for activities that use Sky.
 */
public class SkyActivity extends Activity {
    private FlutterView mView;

    private String[] getArgsFromIntent(Intent intent) {
        // Before adding more entries to this list, consider that arbitrary
        // Android applications can generate intents with extra data and that
        // there are many security-sensitive args in the binary.
        ArrayList<String> args = new ArrayList<String>();
        if (intent.getBooleanExtra("enable-checked-mode", false)) {
            args.add("--enable-checked-mode");
        }
        if (intent.getBooleanExtra("trace-startup", false)) {
            args.add("--trace-startup");
        }
        if (intent.getBooleanExtra("start-paused", false)) {
            args.add("--start-paused");
        }
        if (!args.isEmpty()) {
            String[] argsArray = new String[args.size()];
            return args.toArray(argsArray);
        }
        return null;
    }

    /**
     * @see android.app.Activity#onCreate(android.os.Bundle)
     */
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.LOLLIPOP) {
            Window window = getWindow();
            window.addFlags(WindowManager.LayoutParams.FLAG_DRAWS_SYSTEM_BAR_BACKGROUNDS);
            window.setStatusBarColor(0x40000000);
            window.getDecorView().setSystemUiVisibility(
                    View.SYSTEM_UI_FLAG_LAYOUT_STABLE
                    | View.SYSTEM_UI_FLAG_LAYOUT_FULLSCREEN);
        }

        String[] args = getArgsFromIntent(getIntent());
        FlutterMain.ensureInitializationComplete(getApplicationContext(), args);
        mView = new FlutterView(this);
        setContentView(mView);

        onSkyReady();
    }

    /**
     * @see android.app.Activity#onDestroy()
     */
    @Override
    protected void onDestroy() {
        if (mView != null) {
            mView.destroy();
        }
        // Do we need to shut down Sky too?
        super.onDestroy();
    }

    @Override
    public void onBackPressed() {
        if (mView != null) {
            mView.popRoute();
            return;
        }
        super.onBackPressed();
    }

    @Override
    protected void onPause() {
        super.onPause();
        if (mView != null) {
            mView.onPause();
        }
    }

    @Override
    protected void onPostResume() {
        super.onPostResume();
        if (mView != null) {
            mView.onPostResume();
        }
    }

    /**
      * Override this function to customize startup behavior.
      */
    protected void onSkyReady() {
        TraceEvent.instant("SkyActivity.onSkyReady");

        if (loadIntent(getIntent())) {
            return;
        }
        String appBundlePath = FlutterMain.findAppBundlePath(getApplicationContext());
        if (appBundlePath != null) {
            mView.runFromBundle(appBundlePath, null);
            return;
        }
    }

    protected void onNewIntent(Intent intent) {
        loadIntent(intent);
    }

    public boolean loadIntent(Intent intent) {
        String action = intent.getAction();
        if (Intent.ACTION_RUN.equals(action)) {
            String file = intent.getStringExtra("file");
            String packageRoot = intent.getStringExtra("packages");
            String route = intent.getStringExtra("route");
            // TODO(johnmccutchan): Remove the need for the runFromFile
            // intent by launching an empty application at startup and
            // reloading from within that.
            if ((file != null) && (packageRoot != null)) {
                mView.runFromFile(file, packageRoot);
            } else {
                mView.runFromBundle(intent.getDataString(),
                                    intent.getStringExtra("snapshot"));
            }
            if (route != null)
                mView.pushRoute(route);
            return true;
        }

        return false;
    }
}
