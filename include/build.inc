/*
	Ratojen rakennus
	
*/

/*
	EditTrack
	
	Runko kiskojen tekemiselle. Syötetään datafile element ref, jota muokataan
 */
 
 include ":include:coord";
 
enum RETURN_VALUES
	FUNC_DELETE 	:= -1,
	FUNC_MOVE		:= -2,
	FUNC_DUPE		:= -3,
endenum

function EditTrack( who, track )
	
	var val;
	
	while (who.connected)
		
		var cmds := array;
		
		foreach fu in (DFIntProps(track))
			
			// Pitäisi olla nyt korjattu
			//~ if (fu == "0")
				// Propsi muuttujat eli "Vars". DFIntProps() on string-nimien suhteen puutteellinen
				//~ continue;
			//~ endif
			
			var colour := 500;
			
			if (!ActionIsConfigured(track.GetProp(fu)))
				colour := 338;
			endif
			
			cmds.append({PropToString(track.GetProp(fu)), colour});
		endforeach
		
		cmds.append("ADD NEW");
		cmds.append("SET VARIABLES");
		
		var edited;
		
		// Vastaa suoraa proppiarvoa -> propit säilötään numeroina
		var propname := CStr(AdvListGump(who, 0, cmds, "Functions"));
		
		if (propname == CStr(cmds.size() - 1))
			
			// Lisätään uusi toiminto, valitaan luokka ja sieltä toiminto
			var select := SelectGump(who, function_types, "Select type");
			
			if (!select)
				continue;
			endif
			
			var actions := array;
			
			// Haetaan toimintojen nimet
			foreach action in GetConfigStringKeys(cfg);
				if (cfg[action].type == function_types[select])
					actions.append(action);
				endif
			endforeach
			
			select := SelectGump(who, actions, "Select action");
			
			if (!select)
				continue;
			endif
			
			// -1 on add new ja set variable kohtien takia
			val := EditFunction(who, track, CStr(cmds.size() - 1), actions[select]);
			
		elseif (propname == CStr(cmds.size()))
			// Muuttujien säätö
			var vars := track.GetProp("Vars");
			var res := SelectGump(who, DictToString(vars), "Säädä muuttuja");
			
			if (res)
				var types :=  {"Coords", "Container", "Door", "Item", "Mobile", "Object", "Value", "Int", "String"};
				var newtype := SelectGump(who, types + {"Poista"}, "Vaihda tyyppiä");
				
				if (newtype > types.size())
					// todo: tarkiste onko käytössä
					vars.erase(vars.keys()[res]);
					track.SetProp("Vars", vars);
				elseif(newtype)
					vars[vars.keys()[res]] := types[newtype];
					track.SetProp("Vars", vars);
				endif
			endif
			
		elseif (propname != "0")
			// Poista/Modaa kohta
			val := EditFunction(who, track, propname);
		else
			return;
		endif
		
		var copy_what;
		
		if (val == FUNC_MOVE || val == FUNC_DUPE)
			copy_what := track.GetProp(propname);
		endif
		
		if (val == FUNC_DELETE || val == FUNC_MOVE)
			track.EraseProp(propname);
			
			var i := 1;
			
			// Korjataan numerointi
			foreach fu in (DFIntProps(track))
				
				if (i != CInt(fu))
					track.SetProp(CStr(i), track.GetProp(fu));
					track.EraseProp(fu);
					//~ db("Korjataan "+fu);
				endif
				
				i += 1;
			endforeach
		endif
		
		if (val == FUNC_MOVE || val == FUNC_DUPE)
			var selname := CInt(AdvListGump(who, 0, cmds, "Move where"));
			if (selname > 0)
				
				var oldprops := array;
				var jee := 0;
				
				foreach fu in (DFIntProps(track))
					if (selname == CInt(fu))
						
						jee := selname + 1;
						
						// Asetetaan haluttu prop uuteen paikkaan
						oldprops.append(track.GetProp(fu));
						track.SetProp(fu, copy_what);
						copy_what := 0;
						
						//~ db("Asetetaan "+fu);
					elseif (jee)
						// Korjataan jälkeen tulevat
						oldprops.append(track.GetProp(CStr(jee)));
						track.SetProp(CSTr(jee), oldprops[1]);
						oldprops.erase(1);
						jee += 1;
						
					endif
					
				endforeach
				
				if (copy_what)
					// Tapaus funktion siirto viimeiseksi: Ensin poistetaan, jonka johdosta foreach loppuu ennen asetusta
					track.SetProp(CStr(selname), copy_what);
				else
					// Viiminen pinosta
					track.SetProp(CSTr(jee), oldprops[1]);
				endif
			endif
		endif
		
		// Tyhjätään jottei jää nappi päälle
		val := 0;
		
	endwhile
endfunction


/*
	EditFunction
	
	Säädetään arvot haluttuun toimintoon. Ottaa toisena parametrinä
	raiteen elemreffin ja toisena joko toiminto nimen uuteen toimintoon
	tai vanhan toiminnon nimen
	
	Palauttaa: Uuden/muokatun toimintomuuttuja-paketin
*/
function EditFunction( who, track, propname, newfunc := "" )
	
	var data;
	
	// Uusi toiminto-tietue
	if (newfunc)
		data := dictionary{"Command" -> newfunc};
	else
		data := track.GetProp(propname);
	endif
	
	if (typeof(data) != "Dictionary")
		Message(who, "Propname "+propname +", data on viallinen: "+data, COLOR_ERROR);
		return -1; // Poistetaan
	endif
	
	var elem := cfg[data["Command"]];
	
	var datas := GetConfigStringDictionary(elem, "Data");
	var settings := GetConfigStringDictionary(elem, "Set");
	
	while (who.connected)
		var with_values := array;
		
		// Käydään läpi toiminnon muuttujat
		foreach drum in datas
			
			var temp := SplitWords(drum);
			
			// Jos arvoa ei ole asetettu, merkataan
			if (data[_drum_iter] == error)
				
				// Lisätään default arvo jos määritetty ja ei select-tyyppi
				
				if (temp[1] != "Select" && temp[2])
					
					if (CInt(temp[2]) > 0 || temp[2] == "0")
						// Tuntuisi olevan numero, defaulttaa intiksi
						data[_drum_iter] := CINt(temp[2]);
					else
						// String default arvo
						data[_drum_iter] := temp[2];
					endif
				else
					data[_drum_iter] := "NULL";
				endif
			endif
			
			// Lisätään gumpin valintoihin
			with_values.append("Set " + lower(_drum_iter)+": ("+Lower(temp[1]) +") "+DataToString(drum, data[_drum_iter]));
			
		endforeach
		
		if (!newfunc)
			with_values.append("Move");
			with_values.append("Dupe");
			with_values.append("Move up");
			with_values.append("Move down");
			with_values.append("Delete");
		endif
		
		var select := SelectGump(who, with_values, "Set data values");
		
		if (!newfunc)
			if (select == with_values.size())
				// Poistetaan
				return FUNC_DELETE;
			elseif (select >= with_values.size() - 4)
				
				// Siirrä jonnekin
				if (data)
					track.SetProp(propname, data);
				endif
				
				if (select == with_values.size() - 1)
				
					// Vaihdetaan ylemmän propin kanssa paikkoja
					var oldprop := track.GetProp(CStr(CInt(propname) + 1));
					
					if (oldprop)
						track.SetProp(CStr(CInt(propname) + 1), track.GetProp(propname));
						track.SetProp(propname, oldprop);
						
						Message(who, "Down done.");
					endif
					
					return;
					
				elseif (select == with_values.size() - 2)
					
					// Vaihdetaan alemman propin kanssa paikkoja
					if (CInt(propname) > 1)
						var oldprop := track.GetProp(CStr(CInt(propname) - 1));
						track.SetProp(CStr(CInt(propname) - 1), track.GetProp(propname));
						track.SetProp(propname, oldprop);
						
						Message(who, "Up.");
					endif
					
					return;
					

				elseif (select == with_values.size() - 3)
					
					return FUNC_DUPE;
					
				elseif (select == with_values.size() - 4)
					
					return FUNC_MOVE;
				endif	
			endif
		endif
		
		if (!select)
			break;
		endif
		
		// Etsitään numeron perusteella valittu muuttuja
		var i := 0;
		
		foreach dataname in datas
			
			i += 1;
			
			// Jaiks, nyt täsmää!
			if (i == select)
				var words := SplitWords(dataname);
				
				// Asetetaan selectin vaihtoehdot
				var additional;
				
				if (words[2])
					additional := words;
					additional.erase(1);
				endif
				
				var val := SetDataValue(who, words[1], additional, track, 0, settings[_dataname_iter], data[_dataname_iter]);
				if (val != error)
					data[_dataname_iter] := val;
				endif
				
			endif
			
		endforeach
	endwhile
	
	if (data)
		track.SetProp(propname, data);
		return 1;
	endif
	
endfunction


/*
	SetDataValue
	
	Pyytää käyttäjältä halutun tyyppistä arvoa
	
	Palauttaa: Halutun tyyppisen arvon
*/

include ":music:music";

function SetDataValue( who, type, additional, track, novars, settings := 0, byref current := 0 )
	
	var value;
	
	if (!novars)
		if (settings == "forcevar")
			novars := 4;
		elseif (type in {"Color", "Int", "Music", "Value", "String"})
			case(SelectGump(who, {"Staattinen arvo", "Muuttuja"}, "Aseta.."))
				1:
					break;
				2:
					novars := 4;
				default:
					return error;
			endcase
		endif
	endif
	
	if (novars == 4)
		return SetVariable(track, who, type);
	endif
	
	case (type)
		"Color":
			value := SelectColor(who, who.backpack);
		"Coords":
			
			var result := SelectGUmp(who, {"Pelaaja", "Sijainti", "Objekti", "Objekti + offset", "Muuttuja", "Muuttuja + offset"}, "Käytätkö objektia vai sijaintia?");
			
			case(result)
				1:
					value := "user";
				2:
				3:
					var targ := TargetCoordinates(who);
					
					if (targ)
						if (targ.mobile || targ.item)
							var object;
							if (targ.mobile)
								object := targ.mobile;
							else
								object := targ.item;
							endif
							
							if (result == 3)
								value := object.serial;
							endif
						endif
						
						if (!value)
							value := GetCoords(targ);
						endif
					endif
				5:
					return SetVariable(track, who, type);
				4:
				6:
					var offset := struct{x, y, z};
					
					offset.x := Cint(Request(who, "X"));
					offset.y := Cint(Request(who, "Y"));
					offset.z := Cint(Request(who, "Z"));
					
					if (result == 6)
						var xlol := SetVariable(track, who, type);
						
						if (xlol)
							value := dictionary{"Object" -> xlol, "Offset" -> offset};
						endif
					else
						var targ := TargetCoordinates(who);
						if (targ.mobile || targ.item)
							
							if (targ.mobile) targ := targ.mobile; else targ := targ.item; endif
							
							value := dictionary{"Object" -> targ.serial, "Offset" -> offset};
						endif
					endif
			endcase
			
		"Int":
			// Aseta integer
			value := CInt(Request(who, "Type number."));
		"Music":
			value := SelectMusic(who);
		
		"Container":
		"Door":
		"Item":
		"Mobile":
		"Object":
			var selects := {"Pelaaja", "Vipu", "Objekti", "Useampi objekti", "Useampi objekti ja...", "Muuttuja"};
			
			if (type in {"Mobile", "Object"})
				selects[7] := "Näkyvät lähipelaajat";
				selects[8] := "Kaikki lähipelaajat";
				selects[9] := "Lähin NPC";
			endif
			
			var result := SelectGump(who, selects);
			
			// Aseta objecti
			case (result)
				1:
					value := "user";
				2:
					value := "lever";
				3:
					value := Target(who, TGTOPT_NOCHECK_LOS).serial;
				4:
				5:
					value := array;
					var targ;
					
					var palat := dictionary;
				
					if (TypeOf(current) == "Array")
						value := current;
						
						foreach pala in current
							var what := SystemFindObjectBySerial(pala);
							
							if (what)
								palat[pala] := what.color;
								what.color := 33;
							endif
						endforeach
					endif
					
					do
						targ := Target(who, TGTOPT_NOCHECK_LOS);
						
						if (targ)
							if (targ.serial in value)
								value.erase(IndexByValue(value, targ.serial));
								Private(targ, "Poistettu", who, COLOR_ERROR);
							else
								value.append(targ.serial);
								Private(targ, "Lisätty", who, COLOR_INFO);
								palat[targ.serial] := targ.color;
								targ.color := 33;
							endif
						endif
					dowhile(targ);
					
					foreach pala in palat
						SystemFindObjectBySerial(_pala_iter).color := pala;
					endforeach
					
					if (!value.size())
						value := error;
					elseif (result == 5)
						var also := {"user", "lever", "nearby", "nearbyall"};
						
						while (also.size())
							result := SelectGUmp(who, also, "Lisää myös...");
							
							if (result > 0)
								value.append(also[result]);
								also.erase(result);
							else
								break;
							endif
						endwhile
					endif
				6:
					value := SetVariable(track, who, type);
				7:
					if (type in {"Mobile", "Object"})
						value := "nearby"; // -> range
					else
						// Tähän constilistaus
						value := "group";
					endif
				8:
					value := "nearbyall"; // -> range
				9:
					value := "closest"; // -> range
			endcase
		"Value":
			
			if (current != "NULL")
				var what := InputGump(who, "Type value.", current, 1, 340);
				if (what)
					value := what;
				endif
			else
				value := Request(who, "Type value.");
			endif
			
			var has_alpha := false;
			
			// Etsitään sisältääkö stringi pelkkiä numeroita
			foreach letter in CascZ(value)
				if (letter < 45 || letter > 57)
					has_alpha := true;
					break;
				endif
			endforeach
			
			// Muunnetaan numeroksi
			if (!has_alpha)
				value := CInt(value);
			endif
			
		"String":
			// Aseta string
			
			if (current != "NULL")
				var what := InputGump(who, "Type text.", current, 1, 340);
				if (what)
					value := what;
				endif
			else
				value := Request(who, "Type text.");
			endif
			
		"Bool":
			// Aseta bool
			var result := SelectGump(who, {"True", "False", "Muuttuja"}, "Set boolean value");
			
			case(result)
				1:
					value := 1;
				2:
					value := 0;
				3:
					value := SetVariable(track, who, type);
			endcase
			
		"Select":
			// Aseta listasta
			var result := SelectGump(who, additional + {"Muuttuja"}, "Select value");
			
			if (result > additional.size())
				value := SetVariable(track, who, type);
			elseif (result)
				value := additional[result];
			endif
			
		default:
			value := error{"errortext" := "Virhe datan asetuksessa, tyyppiä "+type+" ei löydy."};
			Db(value);
	endcase
	
	return value;
	
endfunction


function CheckSetting(action, prop, setting)
	
	var cfg := ReadConfigFile(":lever:actions");
	var settings := GetConfigStringDictionary(cfg[action], "Set");
	
	return (settings[prop] == setting);

endfunction

function SetVariable(track, who, type)
	var vars := track.GetProp("Vars");
	var value;
		
	if (!vars)
		vars := dictionary;
	endif
	var res:= SelectGump(who, vars.keys() + {"New variable"});
	
	if (res == vars.size() + 1)
		// uusi
		var req := Request(who, "Name the variable.");
		
		if (req)
			value := VARIABLE_PREFIX + req;
			vars[req] := type;
			track.SetProp("Vars", vars);
		endif
	elseif (res)
		// vanhaa modataan. örr
		res := vars.keys()[res];
		
		if (CheckType(vars[res], type))
			value := VARIABLE_PREFIX + res;
		else
			Message(who, "Tyyppi "+vars[res]+" ei täsmää.");
		endif
	endif
	
	return value;
		
endfunction

/*
	PropToString
	
	
	
	Palauttaa: datan sisällön stringinä muodossa ": prr"
*/

function PropToString( prop )
	//~ db("Rawr.." +prop);
	var text := prop["Command"];
	var cfg_elem := cfg[prop["Command"]];
	var datadicti := GetConfigStringDictionary(cfg_elem, "Data");
	
	// string on varattu sana
	var muotoilija := cfg_elem."string";
	var space := ": ";
	
	foreach muoto in SplitWords(muotoilija)
		
		// Muotoillaan välilyönnit
		if (muoto["<"])
			case (muoto)
				"<no-space>":
					space := "";
				default:
					// Sana välissä
					muoto["<"] := "";
					muoto[">"] := "";
					text += space + muoto;
					space := " ";
			endcase
		else
			// lol
			var temp := DataToString(datadicti[muoto], prop[muoto]);
			if (temp != "NULL")
				text += space + lower(temp);
				space := " ";
			endif
		endif
		
	endforeach
	
	return text;
	
endfunction

/*
	DataToString
	
	Muodostaa tekstin syötetyn tyypin ja datan sisällön perusteella. Huomioi
	ettei tyyppiä säilötä datassa, eikä sitä siitä voi tunnistaa.
	
	Palauttaa: dataa kuvaavaa string
*/

function DataToString( datatype, data )
	//~ db(datatype);
	if (data == error)
		return "";
	elseif (data == "NULL")
		return data;
	endif
	
	var returning := "";
	
	if (typeof(data) == "String")
		if (data[1, 4] == VARIABLE_PREFIX)
			return "VAR "+data[5, len(data)];
		endif
	endif
	var type := TypeOf(data);
	
	// Konfig-entrystä vain alku
	case (SplitWords(datatype)[1])
		"Select":
		"String":
		"Value":
			returning := data;
		"Color":
		"Int":
			returning := CStr(data);
		"Bool":
			returning := Boolean(data);
		"Container":
		"Door":
		"Item":
		"Mobile":
		"Object":
			if (TypeOf(data) == "Array")
				returning := data.size()+" objects";
			elseif (type == "String")
				returning := data;
			else
				data := SystemFindObjectBySerial(data);
				// Tähän taas moni-objekti tuki (constit)
				if (data.isa(POLCLASS_ITEM))
					returning := data.desc;
				else
					returning := data.name;
				endif
			endif
		"Coords":
			if (type == "Integer")
				data := SystemFindObjectBySerial(data);
				
				if (data.name)
					data := "object "+data.name;
				else
					data := "object "+data.desc;
				endif
			elseif (type == "String")
				returning := data;
			elseif (type == "Dictionary")
				returning := "offset "+data["Offset"].x + ", "+data["Offset"].y + ", "+data["Offset"].z;
			else
				returning := data.x + ", " + data.y + ", "+ data.z;
			endif
		default:
			returning := "Unknown: "+CStr(data);
	endcase
	
	return returning;
	
endfunction

/*
	ActionIsConfigured
	
	Nopea rykäsy, ei todellakaan joustava
	
	Palauttaa:
*/

function ActionIsConfigured( actions )
	
	// in tapa ei tue dictejä
	foreach act in actions
		if (act == "NULL")
			return 0;
		endif
	endforeach
	
	return 1;
	
endfunction

/*
	Parametrit:
		tyyppi jota tyrkytetään
		tyyppi jota vaaditaan
*/
function CheckType( type, requirement )
	// HUOM. tuki tähän mikä arvo tukee mitäkin
	//	esim. väriksi kelpaa myös integer,
	//	tai objectiksi mobile ja item
	
	// Pitää miettiä tarpeellisuus vielä uusiksi
	return 1;
	
	if (type == requirement)
		return 1;
	endif
	
	case (requirement)
		"Coords":
			// Kordinaatit saa seuraavista
			return type in {"Container", "Door", "Item", "Mobile", "Object"};
		"Object":
			return type in {"Container", "Door", "Item", "Mobile"};
		"Item":
			return type in {"Container", "Door", "Object"};
	endcase
	
endfunction

