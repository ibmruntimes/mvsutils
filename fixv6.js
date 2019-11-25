/*
 * example to convert a version 6 untag ebcdic file to unicode
 *
 */
const mvsutils = require('./build/Release/mvsutils.node');
module.exports = mvsutils;
var fs = require("fs")
var nospawn = false
var inputfile
var outputfile

if (process.argv.length > 2 && process.argv[2] == 'child') {
  nospawn = true
}

function opened(err, fd) {
  if (err) {
    console.warn("file open error")
    process.exit(-1)
  }
  let res = mvsutils.GuessFileCcsid(fd)
  if (res.ccsid != 1047) {
    console.log(res)
    console.warn("file", inputfile, "does not seem to contain valid EBCDIC data, please inpect output file", outputfile)
  }
  try {
    fs.unlinkSync(outputfile)
  } catch (err) {}

  fs.createReadStream(inputfile).pipe(fs.createWriteStream(outputfile))
  console.log("output file created", outputfile)
}

if (process.env.__UNTAGGED_READ_MODE !== "V6") {
  if (nospawn) {
    console.warn("in child but __UNTAGGED_READ_MODE is not V6")
    process.exit(-1)
  }
  process.env.__UNTAGGED_READ_MODE = "V6"
  const spawn = require('child_process').spawn;
  let me = process.execPath
  const args = [process.argv[1], 'child', process.argv[2]]
  const opts = {
    stdio: 'inherit'
  }
  let child = spawn(me, args, opts);

} else {
  if (nospawn) {
    file = process.argv[3]
  } else {
    file = process.argv[2]
  }
  var result = mvsutils.GetFileCcsid(file)
  if (result.ccsid === 0) {
    fs.open(file, "r", opened)
    inputfile = file
    outputfile = file + "._tmpout"
  } else {
    console.warn("file", file, "is not untagged")
    console.warn(result)
    process.exit(-1)
  }
}
