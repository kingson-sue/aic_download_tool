set com_name=%1
set flash_rate=%2
set bin_path=%3

G:\B_Project\A_Git\B_network_proj\SerialPortYmodem-master\bootx\bootx.exe -m  apus  -t  s  -d  %com_name%  -r  %flash_rate%  -c  download 0 "%bin_path%";reboot