/**
 * Sample React Native App
 * https://github.com/facebook/react-native
 *
 * @format
 * @flow strict-local
 */

import React, {useEffect, useState} from 'react';
import {readFile, writeFile, isFileAvailable} from 'react-native-binary-fs';
import {
  SafeAreaView,
  ScrollView,
  StatusBar,
  StyleSheet,
  Text,
  useColorScheme,
  View,
} from 'react-native';
import DocumentPicker from 'react-native-document-picker';

import {Colors} from 'react-native/Libraries/NewAppScreen';

const App = () => {
  const isDarkMode = useColorScheme() === 'dark';
  // console.log(global.isFileAvailable('test.txt'));
  const pickDocument = async () => {
    // Pick a single file
    try {
      const res = await DocumentPicker.pick({
        mode: 'open',
        type: [DocumentPicker.types.allFiles],
      });
      const uri = res[0].uri;
      const size = Number(res[0].size);
      // console.log(isFileAvailable(uri));
      // console.log(uri);
      // console.log(size);
      // const data = new Uint8Array([116, 101, 115, 116, 101]);
      const response = readFile(uri, size, 0, true);
      // const arr = global.readFile(uri, size, 0, true);
      const isCompressed = response.isCompressed;
      const compressedSize = isCompressed ? response.data.length : size;
      console.log(response.data.buffer);
      const savedUrl = writeFile(
        'teste.txt',
        response.data.buffer,
        size,
        false,
        isCompressed,
        compressedSize,
      );
      console.log(savedUrl);
    } catch (err) {
      if (DocumentPicker.isCancel(err)) {
        // User cancelled the picker, exit any dialogs or menus and move on
      } else {
        throw err;
      }
    }
  };

  const backgroundStyle = {
    backgroundColor: isDarkMode ? Colors.darker : Colors.lighter,
  };

  return (
    <SafeAreaView style={backgroundStyle}>
      <StatusBar barStyle={isDarkMode ? 'light-content' : 'dark-content'} />
      <ScrollView
        contentInsetAdjustmentBehavior="automatic"
        style={backgroundStyle}>
        <View
          style={{
            backgroundColor: isDarkMode ? Colors.black : Colors.white,
          }}>
          <Text onPress={pickDocument}>Teste FILE PICK</Text>
        </View>
      </ScrollView>
    </SafeAreaView>
  );
};

const styles = StyleSheet.create({
  sectionContainer: {
    marginTop: 32,
    paddingHorizontal: 24,
  },
  sectionTitle: {
    fontSize: 24,
    fontWeight: '600',
  },
  sectionDescription: {
    marginTop: 8,
    fontSize: 18,
    fontWeight: '400',
  },
  highlight: {
    fontWeight: '700',
  },
});

export default App;
