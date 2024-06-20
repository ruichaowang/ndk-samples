/*
 * Copyright 2013 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

package com.android.gles3jni;

import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.MotionEvent;
import android.view.WindowManager;

import java.io.File;

public class GLES3JNIActivity extends Activity {

    GLES3JNIView mView;
    float initialX, initialY;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mView = new GLES3JNIView(getApplication());
        setContentView(mView);
    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }

    @Override
    public boolean onTouchEvent(MotionEvent event) {
        // 调用native方法，传递触摸事件参数

        int action = event.getActionMasked();
        switch (action) {
            case MotionEvent.ACTION_DOWN:
                // 记录初始触摸位置
                initialX = event.getX();
                initialY = event.getY();
                break;
            case MotionEvent.ACTION_MOVE:
                // 计算移动的距离
                float deltaX = event.getX() - initialX;
                float deltaY = event.getY() - initialY;
                // 调用native方法，传递触摸事件和位移
                GLES3JNILib.onTouchMove(deltaX, deltaY);
                // 重置初始位置
                initialX = event.getX();
                initialY = event.getY();
                break;
            case MotionEvent.ACTION_UP:
                // 可选：处理手指抬起的事件
                break;
        }

        return super.onTouchEvent(event);
    }
}
