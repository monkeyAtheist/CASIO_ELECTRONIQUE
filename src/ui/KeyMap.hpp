#pragma once
// Conversion des codes de touche gint en touches de l'application.
// ch : chiffre ('0'..'9') pour AK_DIGIT ; letter : lettre rouge (recherche).
int mapGintKey(int key, char &ch, char &letter);
