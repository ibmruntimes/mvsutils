# mvsutils

Node.js interface to z/OS resources

## Installation
<!--
This is a [Node.js](https://nodejs.org/en/) module available through the
[npm registry](https://www.npmjs.com/).
-->
Before installing, [download and install Node.js](https://developer.ibm.com/node/sdk/ztp/).
Node.js 8.16 for z/OS or higher is required.

```bash
npm install mvsutils
```
### Availble interfaces

#### SimpleConsoleMessage
* Arguments: String or type that can be converted to a string via ToString()
e.g
```
mvsutils.SimpleConsoleMessage("Write this to","System console") 
```
#### GetFileCcsid
* First argument: Path name of file descriptor, if path name is a number prepend with ./ to make it a string
* Returns object contains either "fd" or "file" and "ccsid", "text" or "error".
e.g
```
fs.open("./somefile.txt","r",function (err,fd) {
    let attr = mvsutils.GetFileCcsid(fd)
    console.log("file text mode", attr.text, " file ccsid", attr.ccsid)
})
```
or
```
let attr = mvsutils.GetFileCcsid("./sometext.txt")
console.log("file text mode", attr.text, " file ccsid", attr.ccsid)
```
#### SetFileCcsid
* First argument: Path name of file descriptor, if path name is a number prepend with ./ to make it a string
* Second argument: text mode bit in file attribute, 1 or 0
* Third arguemnt: CCSID to set
* Return object contains "rc", "error" may be set
```
fs.open("./somefile.txt","r+",function (err,fd) {
    let result = mvsutils.SetFileCcsid(fd,1,1047)
    console.log(result.rc)
})
```
or
```
let result = mvsutils.SetFileCcsid("./somefile.txt",1,819)
console.log(result.rc)
```
#### GuessFileCcsid
Function to perform a content scan on a file descriptor or path name.
* First argument: Path name of file descriptor, if path name is a number prepend with ./ to make it a string
* Second argument: optional call back function
* Return object contains "fd" or "file", "rc", "error" may be set, "ccsid" contains best guess CCSID of the file.
If the file contains well formed unicode in utf-8, "ccsid" will be set to 819
e.g
```
fs.open("./somefile.txt","r+",function (err,fd) {
    mvsutils.GuessFileCcsid(fd,function(obj){
        if (obj.ccsid) {
            console.log("file ccsid guessed:", obj.ccsid)
        }
    })
})

```
or
```
let result = mvsutils.GuessFileCcsid("./somefile.txt");
if (result.ccsid) {
  console.log("file ccsid: ", result.ccsid)
} else {
  console.log(result)
}

```

### Tests

npm test

### Examples

fixv6.js contains an example to fix untag files created by node.js V6 npm downloads.
