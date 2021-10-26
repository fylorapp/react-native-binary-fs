package com.reactnativebinaryfs;

import android.database.Cursor;
import android.net.Uri;
import android.util.Log;
import androidx.annotation.NonNull;
import android.content.Context;
import android.content.ContentResolver;

import com.facebook.react.bridge.JavaScriptContextHolder;
import com.facebook.react.bridge.Promise;
import com.facebook.react.bridge.ReactApplicationContext;
import com.facebook.react.bridge.ReactContextBaseJavaModule;
import com.facebook.react.bridge.ReactMethod;
import com.facebook.react.module.annotations.ReactModule;

import java.io.File;
import java.io.FileOutputStream;
import java.io.InputStream;
import java.io.OutputStream;

@ReactModule(name = BinaryFsModule.NAME)
public class BinaryFsModule extends ReactContextBaseJavaModule {
  public static final String NAME = "BinaryFs";

  public BinaryFsModule(ReactApplicationContext reactContext) {
    super(reactContext);
  }

  public boolean isFileAvailable(String fileUrl) {
    ContentResolver resolver = getReactApplicationContext().getContentResolver();
    Uri uri = Uri.parse(fileUrl);
    Cursor cursor = resolver.query(uri, null, null, null, null);
    boolean doesExist = (cursor != null && cursor.moveToFirst());
    if (cursor != null) {
      cursor.close();
    }
    return doesExist;
  }

  public byte[] readFile(String fileUrl, int chunkSize, int offset, boolean readAllBytes) {
    try {
      ContentResolver resolver = getReactApplicationContext().getContentResolver();
      Uri uri = Uri.parse(fileUrl);
      InputStream is = resolver.openInputStream(uri);
      byte[] bytes = new byte[chunkSize];
      is.skip(offset);
      is.read(bytes, 0, chunkSize);
      return bytes;
    } catch (Exception e) {
      return null;
    }
  }

  public String writeFile(String fileName, byte[] data, boolean append) {
    try {
      File file = new File(getReactApplicationContext().getExternalFilesDir(null), fileName);
      long size = file.length();
      long offset = 0;
      Uri uri = Uri.fromFile(file);
      if (append) {
        offset = size;
      } else {
        if (file.exists()) {
          file.delete();
          file = new File(getReactApplicationContext().getExternalFilesDir(null), fileName);
        }
      }
      OutputStream os = new FileOutputStream(file);
      os.write(data, (int) offset, data.length);
      os.flush();
      os.close();
      return uri.toString();
    } catch (Exception e) {
      return null;
    }
  }

  @Override
  @NonNull
  public String getName() {
    return NAME;
  }

  static {
    try {
      // Used to load the 'native-lib' library on application startup.
      System.loadLibrary("binaryfs");
    } catch (Exception ignored) {
    }
  }

  private native void nativeInstall(long jsi);

  public void installLib(JavaScriptContextHolder reactContext) {
    if (reactContext.get() != 0) {
      this.nativeInstall(reactContext.get());
    } else {
      Log.e("BinaryFsModule", "JSI Runtime is not available in debug mode");
    }
  }
}
