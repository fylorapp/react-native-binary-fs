declare var global: any;

export function isFileAvailable(fileUrl: string): boolean {
  return global.RNBinaryFs.isFileAvailable(fileUrl);
}

export function readFile(
  fileUrl: string,
  chunkSize?: number,
  offset?: number,
  shouldCompress?: boolean
): Uint8Array {
  if (arguments.length < 1) {
    throw Error('You need to provide the file url');
  }
  if (arguments.length === 1) {
    return global.RNBinaryFs.readFile(fileUrl);
  }
  if (arguments.length === 2) {
    return global.RNBinaryFs.readFile(fileUrl, chunkSize);
  }
  if (arguments.length === 3) {
    return global.RNBinaryFs.readFile(fileUrl, chunkSize, offset);
  }
  return global.RNBinaryFs.readFile(fileUrl, chunkSize, offset, shouldCompress);
}

export function writeFile(
  fileName: string,
  data: Uint8Array,
  originalSize: number,
  append?: boolean,
  isCompressed?: boolean,
  compressedSize?: number
): Uint8Array {
  if (arguments.length < 3) {
    throw Error(
      'You need to provide the file name, the data as an ArrayBuffer and the size of the buffer'
    );
  }
  if (arguments.length === 3) {
    return global.RNBinaryFs.writeFile(fileName, data, originalSize);
  }
  if (arguments.length === 4) {
    return global.RNBinaryFs.writeFile(fileName, data, originalSize, append);
  }
  if (arguments.length === 5 && !isCompressed) {
    return global.RNBinaryFs.writeFile(
      fileName,
      data,
      originalSize,
      append,
      isCompressed
    );
  }
  return global.RNBinaryFs.writeFile(
    fileName,
    data,
    originalSize,
    append,
    isCompressed,
    compressedSize
  );
}
