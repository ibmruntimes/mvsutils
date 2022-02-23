/*
 * Licensed Materials - Property of IBM
 * (C) Copyright IBM Corp. 2019. All Rights Reserved.
 * US Government Users Restricted Rights - Use, duplication or disclosure
 * restricted by GSA ADP Schedule Contract with IBM Corp.
 */

const mvsutils = require("../build/Release/mvsutils.node");
const expect = require('chai').expect;
const should = require('chai').should;
const assert = require('chai').assert;
var fs = require("fs")
file = "console_log_test.txt"

var doner

function open1(err, fd) {
  console.log("Created file ", file)
  if (err) {
    console.error(err)
    expect(false).to.be.true
    return
  }
  let binsh = mvsutils.GetFileCcsid("/bin/sh")
  console.log("File", "/bin/sh", "tagged as", "t:", binsh.text, "ccsid:", binsh.ccsid)
  expect(binsh.text === 0).to.be.true
  expect(binsh.ccsid === 0).to.be.true

  process.env._BPXK_JOBLOG = fd // export _BPXK_JOBLOG=<file descriptor>

  mvsutils.SimpleConsoleMessage("TEST", "READ", "ME") // write console message

  // Get actual file tagging information
  let attr = mvsutils.GetFileCcsid(fd)
  console.log("File", file, "tagged as", "t:", attr.text, "ccsid:", attr.ccsid)

  expect(attr.text === 1).to.be.true
  expect(attr.ccsid === 819).to.be.true

  // test ccsid changing
  let res = mvsutils.SetFileCcsid(file, 0, 65535) // change it to binary
  attr = mvsutils.GetFileCcsid(file)
  console.log("File", file, "tagged as", "t:", attr.text, "ccsid:", attr.ccsid)
  expect(attr.text === 0).to.be.true
  expect(attr.ccsid === 65535).to.be.true

  // scan what the real character encoding should be
  fs.close(fd, reopen1)

  // Make sure a large string does not result in Node.js crashing
  var str = "";
  for (i = 0; i < 2000; ++i) {
    str += " " + i;
  }
  mvsutils.SimpleConsoleMessage(str, str)
}

function checkfile(err) {
  if (err) {
    console.error(err)
    expect(false).to.be.true
    return
  }
  fs.readFile(file, 'utf8', function(err, contents) {
    console.log("file data:", contents);
    if (contents.indexOf('TEST READ ME') >= 0) {
      console.log("SUCCESS: data in correct codepage")
    } else {
      expect(false).to.be.true
    }
    fs.unlinkSync(file)
    doner()
  })
}

function async_scan(obj) {
  let fd = obj.fd
  let ccsid = obj.ccsid // results from scanning content
  expect(ccsid === 1047).to.be.true // expect console messages to be really in 1047
  mvsutils.SetFileCcsid(fd, 1, ccsid)
  let result = mvsutils.GetFileCcsid(fd) // test synchronous mode
  expect(result.ccsid === 1047).to.be.true // expect file ccsid now set to 1047
  fs.close(fd, checkfile)

}

function open2(err, fd) {
  console.log("Open file", file, "for read")
  if (err) {
    console.error(err)
    expect(false).to.be.true
    return
  }
  let attr = mvsutils.GuessFileCcsid(fd, async_scan)
}

function reopen1(err) {
  fs.open(file, "r", open2)
}

describe("mvsutils phase 1 Validation",
  function() {
    it(
      "phase 1 test",
      function(done) {
        try {
          fs.unlinkSync(file)
        } catch (err) {}
        fs.open(file, "w+", open1)
        doner = done
      })

  });
