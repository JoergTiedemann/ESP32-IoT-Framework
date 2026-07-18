import json
import binascii

import inspect, os.path

def preBuildDashFun():
    filename = inspect.getframeinfo(inspect.currentframe()).filename
    dir_path = os.path.dirname(os.path.abspath(filename))

    filename = "dash"
    h = open(dir_path + "/../src/generated/" + filename + ".h", "w", encoding="utf8")

    with open(dir_path + '/../gui/js/dashboard.json') as f:
        data = json.load(f)

    # binascii.crc32(mes.encode('utf8'))
    #headers
    h.write("#ifndef DASH_H\n")
    h.write("#define DASH_H\n\n")
    h.write("struct dashboardData\n{\n")   

    #loop through variables
    for item in data:        
        if item['type'] != 'separator' and item['type'] != 'label' and item['type'] != 'header': 
            if item['type'] == 'char':
                h.write("\tchar " + item['name'] + "[" + str(item['length']) + "];\n")
            elif item['type'] == 'bool':
                h.write("\t" + item['type'] + " " + item['name'] +";\n")
            elif item['type'] == 'historic':
                print("*********************************************************************************************************")
                print("*********************************************************************************************************")
                print("Attention: historicdata used! Only 1 historicdata array is allowed !, it has to be send after dashdata! ")
                print("*********************************************************************************************************")
                print("*********************************************************************************************************")
            elif item['type'] == 'historic1' or item['type'] == 'historic2':
                print("*********************************************************************************************************")
                print("*********************************************************************************************************")
                print("Attention: historicdata used! Exactly 2 historicdata arrays must be send after dashdata! ")
                print("*********************************************************************************************************")
                print("*********************************************************************************************************")
            else:
                h.write("\t" + item['type'] + " " + item['name'] +";\n")

    #footers    
    h.write("};\n\n#endif")

    h.close()