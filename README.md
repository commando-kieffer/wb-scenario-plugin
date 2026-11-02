# CK - Configuration Scénario

Plugin Workbench (Arma Reforger Tools) pour préparer un scénario CK :

1. ajoute les dépendances CK au projet : CK Core, CK Props, CK Structures (+ d'autres mods si besoin) ;
2. demande le terrain, le nom et la date de l'opération ;
3. renomme le mod `CK_Op_<année>_<mois>_<jour>` (ex. `CK_Op_2026_11_2`) et redémarre Workbench si besoin ;
4. crée le monde du scénario dans le mod : `Worlds/CK_Op_2026_11_2/CK_Op_2026_11_2.ent` (sous-scène du terrain) ;
5. lance Game Mode Setup, toujours en Game Master, avec un mission header au nom de l'opération ;
6. crée le briefing et les loadouts CK du scénario, place les éléments CK, les arsenaux et le point de spawn « Base avancée », puis enregistre le monde.

## Prérequis

- Un projet de scénario créé dans Workbench (le nom n'a pas d'importance, le plugin le renomme).
- Les mods CK Core, CK Props et CK Structures installés (Workshop ou dossier ajouté au launcher Workbench).

## Installation

1. Copie le dossier `Scripts` de ce dossier dans le dossier du projet du scénario, à côté de `addon.gproj`. Si le projet a déjà un dossier `Scripts`, les deux se fusionnent.
   Résultat attendu :
   ```
   <projet>\Scripts\WorkbenchGame\CK_ConfigurationScenario\CK_ConfigurationScenarioPlugin.c
   ```
2. Ouvre le projet dans Workbench. S'il était déjà ouvert : **Plugins > Settings > Reload WB Scripts**, ou redémarre Workbench.
3. Le plugin apparaît dans le menu **Plugins** de la fenêtre principale et du World Editor : **CK - Configuration Scénario**.

Le plugin doit être dans le projet du scénario lui-même : Workbench ne compile que les scripts du projet ouvert et de ses dépendances.

## Utilisation

Lance **Plugins > CK - Configuration Scénario**. La première fois dans une session, Workbench demande d'autoriser le script (il lit et modifie `addon.gproj`) : accepte. Si tu as refusé, redémarre Workbench.

1. **Dépendances** : les 3 mods CK sont ajoutés au projet. Réponds **Oui** pour ajouter d'autres mods, et colle leurs GUID (un par ligne, ou séparés par des virgules).
2. **Terrain** : choisis le monde du terrain (`.ent`), par ex. Everon ou Arland. Le scénario sera une sous-scène de ce terrain.
3. **Opération** : nom de l'opération et date. Le plugin affiche le nom du mod obtenu : **Valider**, ou **Modifier** pour corriger.
4. **Redémarrage** : le dossier du mod est renommé quand Workbench est fermé. Clique sur **Redémarrer Workbench** (enregistre ton travail avant). Workbench rouvre le projet renommé et le plugin reprend tout seul (sinon, relance-le et clique sur **Continuer**).
5. **Monde** : le plugin crée et ouvre `Worlds/<mod>/<mod>.ent`.
6. **Game Mode Setup** : l'assistant vanilla démarre directement au scan du monde (modèle Game Master imposé). Clique sur **Scan world**, **Create entities**, puis **Next** jusqu'au bout. Le mission header est créé au nom de l'opération.
7. **Éléments** : ils sont placés devant la caméra du World Editor et restent sélectionnés, pour les déplacer d'un coup. Le monde est enregistré. La dernière fenêtre liste les fichiers à modifier ; **Ouvrir le briefing** l'ouvre dans le Resource Manager.

Ce qui est déjà dans le monde n'est pas placé une deuxième fois : tu peux relancer le plugin sans risque.

## Fichiers du scénario (dans le mod)

| Fichier | À modifier |
|---|---|
| `Worlds/<mod>/<mod>.ent` | le monde du scénario |
| `Missions/<mod>.conf` | mission header : nom (= opération), description, nombre de joueurs, images |
| `Configs/CK/<mod>_Briefing.conf` | briefing du menu CK : titre (= opération), sous-titre avec la date, Situation / Mission / Exécution / Logistique / Transmissions à compléter |
| `Configs/CK/<mod>_Loadouts.conf` | loadouts partagés du menu CK (vide au départ) |
| entité `CK_Troops` du monde | troops et bordées (CK Core n'a pas de `.conf` pour les troops : tout se règle sur l'entité) |

Le briefing et les loadouts sont reliés à `CK_MissionSettings` (composants Briefing et Loadouts), et le briefing aussi au mission header (champ CK Briefing).

## Éléments placés

| Élément | Mod | Prefab |
|---|---|---|
| Système des troops | CK Core | `Prefabs/CK/CK_Troops.et` |
| Infos de mission du menu CK (briefing, loadouts) | CK Core | `Prefabs/CK/CK_MissionSettings.et` |
| Estrade avec insigne CK | CK Props | `Prefabs/Props/Military/Camps/Platform_01_CK.et` |
| Dépôt de véhicules - héliport | CK Structures | `Prefabs/Structures/Military/Garages/CK_Vehicle_Depot/CK_Vehicle_Depot_Helipad.et` |
| Dépôt de véhicules - plateforme | CK Structures | `Prefabs/Structures/Military/Garages/CK_Vehicle_Depot/CK_Vehicle_Depot_Platform.et` |
| Arsenaux armes, munitions, médical, équipement | Arma Reforger | `Prefabs/Props/Military/Arsenal/AmmoBoxes/US/AmmoBoxArsenal_*_US.et` |
| Point de spawn « Base avancée » | Arma Reforger | `Prefabs/MP/Spawning/SpawnPoint_US.et` |

Point de spawn : en Game Master, les factions ne sont pas jouables au départ (le GM doit les activer). Le plugin rend la faction US jouable dans le gestionnaire de factions du mode de jeu, pour que les joueurs puissent se déployer sur « Base avancée » dès le début.

## Bon à savoir

- Game Mode Setup ne regarde que la sous-scène : sur un terrain qui a déjà une carte (Arland, Everon…), il en crée une deuxième (erreur « Multiple map entities present! » dans la console). Le plugin supprime ce doublon, ainsi que ceux de l'AI World et du Perception Manager si le terrain en a déjà.
- Le menu Plugins contient aussi **CK - Game Mode Setup (Game Master)** : c'est l'assistant vanilla sans le choix du modèle, utilisé par le plugin. Il peut servir seul.
- Après le renommage, le launcher Workbench garde l'ancien chemin du projet : ajoute le nouveau dossier avec **Add Existing**.
- Avant de modifier `addon.gproj`, le plugin en garde une copie dans `Documents\My Games\ArmaReforgerWorkbench\profile\CK_ConfigurationScenario\` (avec l'état de la configuration en cours).
- Le dossier du plugin peut rester dans le projet publié : le jeu ne charge pas les scripts `WorkbenchGame`.
- Mise à jour : remplace simplement le fichier `.c`.
- Testé avec Arma Reforger Tools 1.8.
