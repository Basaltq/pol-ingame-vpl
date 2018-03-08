/*
	Apufunktiot
	
*/

include ":include:datafile";

const DFILE_FUNCTION:= ":lever:function";
const DFILE_APPLICATION := ":lever:application";

enum LEVER_STATUSES
	STATUS_UNUSED	:= 0,
	STATUS_USED	:= 1
endenum

enum FUNCTION_TYPES
	TYPE_ACTION		:= "Action",
	TYPE_COMMAND	:= "Command",
	TYPE_EFFECT		:= "Effect",
endenum

const VARIABLE_PREFIX := "var_";

var function_types := {TYPE_ACTION, TYPE_COMMAND, TYPE_EFFECT};


function GetLeverControl( item )

	// Onko vipu jo käytössä, haetaan kontrolli-skripti
	var controller := item.GetProp("ControlPid");
	
	if (controller)
		controller := GetProcess(controller);
		
		if (controller)
			return controller;
		else
			// Vipuun jäänyt vanha cprop! Ei pitäisi tapahtua.
			DB("Invalid lever-pid saved in cprop. Serial: "+item.serial);
			item.EraseProp("ControlPid");
		endif
	endif
	
	return 0;
	
endfunction


function GetLeverInfo( item )

	var datafile := DFOpen(DFILE_APPLICATION, DF_KEYTYPE_INTEGER);
	var dataid := item.GetProp("LeverID");
	
	if (dataid == error)
		
		return error{"errortext" := "Lever is uninitialized."};
	endif
	
	return datafile.FindElement(dataid);

endfunction


function GetTrack( trackname )

	var dfile := DFOpen(DFILE_FUNCTION, DF_KEYTYPE_STRING);
	var dataid := dfile.FindElement(trackname);
	
	if (dataid == error)
		return error{"errortext" := "No such track excist."};
	endif
	
	return dataid;

endfunction


/*
	AppRemoveTrigger
	
	Tuhoaa datafile-applikaation kun viimeinen linkitetty vipu poistetaan.
*/
function AppRemoveTrigger( lever )
	
	var info := GetLeverInfo(lever);
	
	if (!info)
		return 1;
	endif
	
	var triggers := array;
	
	// Tarkistetaan kytkin tilanne
	foreach item in (info.GetProp("Triggers"))
		if (item != lever.serial)
			if (SystemFindObjectBySerial(item))
				triggers.append(item);
			endif
		endif
	endforeach
	
	if (triggers.size() > 0)
		// Vielä löytyy vipuja kytkettynä sovellukseen, ok
		info.SetProp("Triggers", triggers);
		return 1;
	endif
	
	// Viimeinen tai ainoa sovellukseen kytketty vipu > tuhotaan myös datastore merkinnät
	var dfile := DFOpen(DFILE_APPLICATION, DF_KEYTYPE_INTEGER);
	var id := lever.GetProp("LeverID");
	
	return dfile.DeleteElement(id);
	
endfunction


function SaveVariable(byref event, varname, value)
	
	if (varname[1, 4] == VARIABLE_PREFIX)
		varname := varname[5, len(varname)];
	endif
	
	var dfile := OpenDataFile(DFILE_APPLICATION);
	var elem := dfile.FindElement(event["Lever"].GetProp("LeverID"));
	var vars := elem.GetProp("Vars");
	
	if (!vars)
		vars := dictionary;
	endif
	
	if (!vars[event["TrackID"]])
		vars[event["TrackID"]] := dictionary;
	endif
	vars[event["TrackID"]][varname] := value;
	
	elem.SetProp("Vars", vars);
	
endfunction


function GetVariable(byref event, varname)
	
	if (varname[1, 4] == VARIABLE_PREFIX)
		varname := varname[5, len(varname)];
	endif
	
	var dfile := OpenDataFile(DFILE_APPLICATION);
	var elem := dfile.FindElement(event["Lever"].GetProp("LeverID"));
	var vars := elem.GetProp("Vars");
	
	if (!vars[event["TrackID"]][varname])
		return 0;
	endif
	
	return vars[event["TrackID"]][varname];
	
endfunction
