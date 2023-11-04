machine = new WEBHMI.Machine({
  port: 8000,
  ipAddress: '127.0.0.1',
  maxReconnectCount: 5000
});
setInterval(WEBHMI.updateHMI, 30)

machine.setReadGroupMaxFrequency('global', 50)

machine.initCyclicRead('MAIN.MyFub')

let writeStructure = function (tag, value) {
  machine.MAIN.MyFub._STRING = "Test 1";
  machine.MAIN.MyFub.TestPropGetSet = 1;
  machine.writeVariable('MAIN.MyFub')
}