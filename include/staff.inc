include ":lever:build";

function ControlPanel( who, item )
	
	var func_data := DFOpen(DFILE_FUNCTION, DF_KEYTYPE_STRING);
	var app_data := DFOpen(DFILE_APPLICATION, DF_KEYTYPE_INTEGER);
	var itemid := item.GetProp("LeverID");
	var elem;
	
	if (itemid)
		elem := app_data.FindElement(itemid);
	endif
	
	if (!elem)
		// TODO pid‰ kirjaa miss‰ kohtaa menn‰‰n, nyt bugaa jos appseja katoaa v‰list‰
		var nextfree := app_data.FindElement(0);
		if (!nextfree)
			nextfree := app_data.CreateElement(0);
			nextfree.SetProp("NextFree", 1);
		endif
		
		var id;
		
		// DoWhile varmuuden vuoksi, jos kuitenkin on varattu
		do
			id := nextfree.GetProp("NextFree");
			nextfree.SetProp("NextFree", id + 1);
		dowhile (app_data.FindElement(id));
		
		DB("Sovellusta ei m‰‰ritelty, luodaan: "+id);
		
		elem := app_data.CreateElement(id);
		item.SetProp("LeverID", id);
		
		// M‰‰rit‰ sovelluksen alkupropsit t‰h‰n
		elem.SetProp("Phase", 1);
		elem.SetProp("TrackIDs", array);
		//~ elem.SetProp("ItemGroups", array);
		elem.SetProp("Triggers", {item.serial});
	endif
	
	// Alustamattomat radat pit‰isi saada n‰kym‰‰n punaisena
	var tracks;
	
	while (1)
		
		tracks := array;
		
		foreach id in (elem.GetProp("TrackIDs"))
			tracks.append(_id_iter+". TRACK: "+id);
		endforeach
		
		var select := SelectGump(who, {"Use lever", "Add new track", "Link other levers to this (overwrites)", "Set use sound"} + tracks, "Vipu - vaiheessa "+elem.GetProp("Phase"), 340);
		
		case (select)
			1:
				UseLever(who, item);
				return;
			2:
				// Lis‰t‰‰n uusi rata
				
				var tracks := elem.GetProp("TrackIDs");
				
				var select := AdvListGump(who, 0, func_data.keys(), "Valkkaa rata");
				
				if (select)
					elem.SetProp("TrackIDs", tracks + func_data.keys()[select]);
				endif
			3:
				// Ooh, hyl‰t‰‰n sovellus!
				var new := Target(who);
				if (new.objtype >= 0x5250 && new.objtype <= 0x5255)
					AppRemoveTrigger(new);
					new.SetProp("LeverID", itemid);
					Private(new, "Copied setting to here.", who);
				else
					Private(new, "Not valid.", who);
				endif
				
				//~ if (YesNo(who, "Sure to remove?");
			4:
				var numero := Request(who, "Numero?");
				
				if (numero)
					item.SetProp("Sound", cint(numero));
				endif
				// Isket‰‰n omaa saundia vivun v‰‰ntˆˆn
			default:
				if (select > 4)
					// Poistetaan, asetetaan objektitietous tai..
					var trackid := elem.GetProp("TrackIDs")[select - 4];
					var trackelem := func_data.FindElement(trackid);
					var sels := {"Poista rata"};
					
					if (trackelem.GetProp("Vars"))
						sels.append("Aseta objektitietous");
					endif
					
					case (SelectGump(who, sels))
						2:
							var appvars := elem.GetProp("Vars");
							var funcvars := trackelem.GetProp("Vars");
							
							if (!appvars)
								appvars := dictionary;
							endif
							
							if (!appvars[trackid])
								appvars[trackid] := dictionary;
							endif
							
							var list := funcvars.keys();
							
							foreach kohta in list
								kohta += " : " + appvars[trackid][kohta];
							endforeach
							
							var res := SelectGump(who, list);
							
							if (res)
								// Valittu muuttuja
								var chose := funcvars.keys()[res];
								
								res := SetDataValue(who, funcvars[chose], 0, trackelem, 1);
								
								if (res != error)
									// Asetetaan valittuun rataan muuttuja nimi -> tyyppi
									appvars[trackid][chose] := res;
									elem.SetProp("Vars", appvars);
								endif
							endif
							
						1:
							// Poistetaan vanha rataa!
							var tracks := elem.GetProp("TrackIDs");
							tracks.erase(select - 4);
							elem.SetProp("TrackIDs", tracks);
					endcase
				else
					return;
				endif
		endcase
		
	endwhile
/* 	
 * 	- Toimintoradat (1, 2, 3, 4 > muokkaa, poista. lis‰‰)
 * 	- Laukaisinmekanismi
 * 	- Linkitetyt vivut
 * 	
 * 	
 */
endfunction

