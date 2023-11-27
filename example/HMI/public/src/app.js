machine = new LUX.Machine({
  port: 8000,
  ipAddress: '127.0.0.1',
  maxReconnectCount: 5000
});
setInterval(LUX.updateHMI, 30)

machine.setReadGroupMaxFrequency('global', 50)

machine.initCyclicRead('MAIN.MyFub')

let writeStructure = function (tag, value) {
  machine.MAIN.MyFub._STRING = "Test 1";
  machine.MAIN.MyFub.TestPropGetSet = 1;
  machine.MAIN.MyFub.TestPropGet = 1;
  machine.MAIN.MyFub.Incrementing = 1000;
  machine.MAIN.MyFub._REAL_Array[0] = 0;
  machine.MAIN.MyFub._REAL_Array[1] = 1;
  machine.MAIN.MyFub._REAL_Array[2] = 2;
  machine.MAIN.MyFub._REAL_Array[3] = 3;
  machine.writeVariable('MAIN.MyFub')
}