#!/usr/bin/python3

import xml.etree.ElementTree as ET


def findCommonPrefix(string1, string2):
    length = min( len(string1), len(string2) )
    for i in range(length):
      if string1[i] != string2[i] :
        return string1[0:i-1]
    if len(string1) < len(string2) :
      return string1
    else :
      return string2

def parseDirectory(directory, cwd, stripDescriptionPrefix) :
      
  for elem in directory.findall("{https://github.com/ChimeraTK/ApplicationCore}variable") :
    varname = elem.attrib["name"]
    vartype = elem.find("{https://github.com/ChimeraTK/ApplicationCore}value_type").text
    vardirection = elem.find("{https://github.com/ChimeraTK/ApplicationCore}direction").text
    varunit = elem.find("{https://github.com/ChimeraTK/ApplicationCore}unit").text
    vardescription = elem.find("{https://github.com/ChimeraTK/ApplicationCore}description").text
    if not vardescription :
      vardescription = ""
    varlength = int(elem.find("{https://github.com/ChimeraTK/ApplicationCore}numberOfElements").text)
    
    if vardirection == "application_to_control_system" :
      thetype=vartype+" (ro)"
    else :
      thetype=vartype
    if varlength > 1 :
      thetype=thetype+" ("+str(varlength)+" elements)"

    print("| "+varname+" | "+vartype+" | "+thetype+" | "+vardescription[stripDescriptionPrefix:]+" |")

  for elem in directory.findall("{https://github.com/ChimeraTK/ApplicationCore}directory") :
    dirdescription = ""
    for subelem in elem.findall("{https://github.com/ChimeraTK/ApplicationCore}variable") :
      vardescription = subelem.find("{https://github.com/ChimeraTK/ApplicationCore}description").text
      if not vardescription :
        vardescription = ""
      if dirdescription == "" :
        dirdescription = vardescription
      else :
        dirdescription = findCommonPrefix(dirdescription, vardescription)

    print("^ "+cwd+"/"+elem.attrib["name"]+" - "+dirdescription[:-2]+" ||||")

    parseDirectory(elem, cwd+"/"+elem.attrib["name"], len(dirdescription)+1)
      

print("^ PV name           ^ Type          ^ Unit    ^ Description   ^")
    
tree = ET.parse("llrfctrl.xml")
root = tree.getroot()
parseDirectory(root,"",0)
