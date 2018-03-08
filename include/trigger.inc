

function UseLever( who, item )
	
	var info := GetLeverInfo(item);
	
	// Jos vaiheita kaksi, vipu jää päätyyn
	AnimateLever(item, (info.GetProp("TrackIDs").size() == 2));
	
	if (GetLeverControl(item))
		Message(who, "The lever is already operating.");
	else
		
		var current := info.GetProp("Phase");
		
		if (info == error || current == error)
			Message(who, info.errortext);
			return error;
		endif
		
		// Rata suoriutuu loppuun vaikka hahmo loggaisi
		SetScriptController(0);
		
		if (RunTrack(info.GetProp("TrackIDs")[current], item, who) == error)
			Message(who, "The lever seems to be broken.");
		endif
	endif
	
endfunction

/*
	Funktio
	
	
	
	Palauttaa:
*/

function AnimateLever( lever, stay := 0 )
	
	var sound := lever.GetProp("Sound");
	
	if (sound)
		PlaySoundEffect(lever, sound);
	endif
	
	var interval := 800;
	var animation := {};
	
	case (lever.graphic)
		0x108C:
		0x108E:
			animation := {0x108C, 0x108D, 0x108E};
		0x108F:
		0x1090:
			animation := {0x108F, 0x1090};
		0x1091:
		0x1092:
			animation := {0x1091, 0x1092};
		0x1093:
		0x1095:
			animation := {0x1093, 0x1094, 0x1095};
		default:
			// EI animaatiota grafiikalle
			return 1;
	endcase
	
	// Kiepautetaan ympäri
	if (lever.graphic in {0x108E, 0x1090, 0x1902, 0x1095})
		animation.reverse();
	endif
	
	// Vipu käy vain päädyssä
	if (!stay)
		var temp := animation;
		temp.reverse();
		temp.erase(1);
		animation += temp;
		interval /= 2;
	endif
	
	// Vivussa on jo tämä grafiikka
	animation.erase(1);
	
	if (!sound)
		if (animation.size() > 1)
			PlaySoundEffect(lever, 0x3E5);
		else
			PlaySoundEffect(lever, 0x4D3);
		endif
	endif
	
	foreach graph in animation
		lever.graphic := graph;
		sleepms(interval);
	endforeach
	
	
	return 1;
	
endfunction
