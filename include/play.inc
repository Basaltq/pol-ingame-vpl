/*
	Kehityshuomautuksia:
		- Toiminto datapaketit vaihtunut structista dictionaryksi,
		dictien joustavuuden takia. ToString() muodostus vaatii näitä
		ainakin, kun voi välittää avaimen muuttujana.
		
	Liikuttelefunktioihin voisi pistää option:
		- fixed, jolloin liikerata on aina sama (jos objekti on missä ei pitäisi, siirretään se ensin lähtöpisteeseen)
		- variable, jolloin liikerata lähtee siitä missä objekti on
*/

var elemApp;

function RunTrack( trackname, lever := 0, user := 0 )
	
	var dfApp := DFOpen(DFILE_APPLICATION, DF_KEYTYPE_INTEGER);
	elemApp := dfApp.FindElement(lever.GetProp("LeverID"));
	
	var track := GetTrack(trackname);
	var doskip := 0;
	
	if (track == error)
		return track;
	else
		
		// Haetaan radan vaiheet
		MAIN: foreach func in DFIntProps(track)
			
			if (doskip == -1)
				break;
			elseif (doskip == -2)
				detach();
				elseif (doskip > 1)
				doskip -= 1;
				continue;
			endif
			
			func := track.GetProp(func);
			var cfgelem := cfg[func["Command"]];
			
			func["User"] := user;
			func["Lever"] := lever;
			func["TrackID"] := trackname;
			
			// Muuta tiedot käytettävään muotoon
			TranslateObjects(func);
			var fname := lower(func["Command"]);
			
			// Turhat tiedot pois
			func.erase("Command");
			
			doskip := Run_Script(":lever:function/"+fname, func);
		endforeach
		
	endif
	
	if (elemApp.GetProp("Phase") != Cint(trackname))
		var nexttrack := cint(trackname) + 1;
		
		if (!elemApp.GetProp(cstr(nexttrack)))
			nexttrack := 1;
		endif
		
		elemApp.SetProp("Phase", nexttrack);
	endif
	
endfunction

function TranslateObjects( byref func )
	
	var cfg := ReadConfigFile(":lever:actions");
	var elem := cfg[func["Command"]];
	var frame := GetConfigStringDictionary(elem, "Data");
	var settings := GetConfigStringDictionary(elem, "Set");
	
	// käydään funktion muuttujat läpi ja tulkataan objektit tietotyyppien perusteella
	foreach fun in frame
		
		if (settings[_fun_iter] != "forcevar")
			CheckVar(func[_fun_iter], func["TrackID"]);
		endif
		
		case (SplitWords(fun)[1])
			"Coords":
			"Object":
			"Mobile":
			"Item":
			"Door":
			"Container":
				GetObject(func[_fun_iter], func);
				
				case (settings[_fun_iter])
					"single":
						// Jos määritelty vaativan vain yhden objektin, ottaa ensimmäisen ryhmästä
						if (func[_fun_iter].size())
							func[_fun_iter] := func[_fun_iter][1];
						endif
					"multiple":
						// Ja päinvastoin
						if (!func[_fun_iter].size())
							func[_fun_iter] := {func[_fun_iter]};
						endif
				endcase
		endcase
		
	endforeach
	
endfunction

/*
	GetObject
	
	
	
	Palauttaa:
*/

function GetObject( byref object, byref func )
	
	// Olisi parempi jos objektin tyypin välittäisi meta-datana,
	//	sen sijaan että luottaa POLin omiin tyyppeihin. Eli todo
	var tyyppi := TypeOf(object);
	
	if (tyyppi == "Dictionary")
		// Kyseessä kordinaatti offset
		
		CheckVar(object["Object"], func["TrackID"]);
		GetObject(object["Object"], func);
		var obj := object["Object"];
		
		if (typeof(obj) == "Array")
			obj := obj[1];
		endif
		
		object := object["Offset"];
		object.x += obj.x;
		object.y += obj.y;
		object.z += obj.z;
		object.+ realm := obj.realm;
		
	elseif (tyyppi == "Array")
		// Kyseessä object group
		var uusiobject := array;
		
		// Tsekataan perästä viimiset, voi olla user, lever, nearby
		var k;
		
		for (k := object.size(); k > 0; k -= 1)
			if (typeof(object[k]) == "String")
				GetObject(object[k], func);
				uusiobject.append(object[k]);
				object.erase(k);
			else
				break;
			endif
		endfor
		
		foreach kohta in object
			var obj := SystemFindObjectBySerial(kohta, SYSFIND_SEARCH_OFFLINE_MOBILES);
			if (obj)
				uusiobject.append(obj);
			endif
		endforeach
		
		if (uusiobject.size())
			object := uusiobject;
		endif
		
	elseif (tyyppi in {"String", "Integer"})
		
		// TODO: säädettäväksi nearbyn objekti
		case (object)
			"user":		object := func["User"];
			"lever":	object := func["Lever"];
			"closest":	object := ClosestMobile(func["User"]);
			"nearby":		object := ListNearbyMobiles(func["Lever"], DEFAULT_RANGE, REQ_PLAYER);
			"nearbyall":	object := ListNearbyMobiles(func["Lever"], DEFAULT_RANGE, REQ_PLAYER, 1 + 2 + 4 + 8);
			default:	
				var temp := SystemFindObjectBySerial(object, SYSFIND_SEARCH_OFFLINE_MOBILES);
				
				if (temp)
					object := temp;
				endif
		endcase
	endif
	
endfunction


/*
	CheckVar
	
	
	Palauttaa:
		syötetyn objectin, mahdollinen var käännettynä
*/

function CheckVar( byref object, trackid )
	
	if (TypeOF(object) != "String")
		return;
	endif
	
	if (object[1, 4] == VARIABLE_PREFIX)
		
		var vars := elemApp.GetProp("Vars");
		
		var muuttujanimi:=object[5, len(object)];
		object := vars[trackid][muuttujanimi];
	endif
	
endfunction
