$port = new-Object System.IO.Ports.SerialPort COM4
$port.ReadTimeout = 1000
$port.Open()
while ($true) {try {$port.ReadLine()} catch {}}