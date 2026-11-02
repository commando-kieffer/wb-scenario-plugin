# CK - Configuration Scénario

Plugin Workbench (Arma Reforger Tools) pour préparer un scénario CK. Avec lui, vous :

1. ajoutez au projet les dépendances CK : CK Core, CK Props, CK Structures (et d'autres mods si besoin) ;
2. choisissez le terrain, puis donnez le nom et la date de l'opération ;
3. renommez le mod `CK_Op_<année>_<mois>_<jour>`, mois et jour sur 2 chiffres (le 2 décembre 2026 donne `CK_Op_2026_12_02`), avec un redémarrage de Workbench si besoin ;
4. créez le monde du scénario dans le mod : `Worlds/CK_Op_2026_12_02/CK_Op_2026_12_02.ent` (sous-scène du terrain) ;
5. créez les calques de l'opération : `Composants_Mission`, `Base_Avancee`, `Jour_1` à `Jour_4` ;
6. lancez Game Mode Setup, toujours en Game Master, avec un mission header au nom de l'opération ;
7. placez les éléments CK, les arsenaux et le point de spawn « Base avancée », avec un briefing et des loadouts CK propres au scénario. Le monde est enregistré à la fin.

## Prérequis

- Un projet de scénario créé dans Workbench (le nom n'a pas d'importance, le plugin le renomme).
- Les mods CK Core, CK Props et CK Structures installés (Workshop ou dossier ajouté au launcher Workbench).

## Installation

1. Copiez le dossier `Scripts` de ce dossier dans le dossier du projet du scénario, à côté de `addon.gproj`. Si le projet a déjà un dossier `Scripts`, les deux se fusionnent.
   Résultat attendu :
   ```
   <projet>\Scripts\WorkbenchGame\CK_ConfigurationScenario\CK_ConfigurationScenarioPlugin.c
   ```
2. Ouvrez le projet dans Workbench. S'il était déjà ouvert : **Plugins > Settings > Reload WB Scripts**, ou redémarrez Workbench.
3. Le plugin apparaît dans le menu **Plugins** de la fenêtre principale et du World Editor : **CK - Configuration Scénario**.

Le plugin doit être dans le projet du scénario lui-même : Workbench ne compile que les scripts du projet ouvert et de ses dépendances.

## Utilisation

Lancez **Plugins > CK - Configuration Scénario**. La première fois dans une session, Workbench demande d'autoriser le script (il lit et modifie `addon.gproj`) : acceptez. Si vous avez refusé, redémarrez Workbench.

1. **Dépendances** : les 3 mods CK sont ajoutés au projet. Répondez **Oui** pour ajouter d'autres mods, et collez leurs GUID (un par ligne, ou séparés par des virgules).
2. **Terrain** : choisissez le monde du terrain (`.ent`), par ex. Everon ou Arland. Le scénario sera une sous-scène de ce terrain.
3. **Opération** : saisissez le nom de l'opération et choisissez la date. Le plugin affiche le nom du mod obtenu : cliquez sur **Valider**, ou sur **Modifier** pour corriger.
4. **Redémarrage** : le dossier du mod est renommé quand Workbench est fermé. Cliquez sur **Redémarrer Workbench** (enregistrez votre travail avant). Workbench rouvre le projet renommé et le plugin reprend tout seul (sinon, relancez-le et cliquez sur **Continuer**).
5. **Monde et calques** : le plugin crée et ouvre `Worlds/<mod>/<mod>.ent`, puis crée les calques de l'opération.
6. **Game Mode Setup** : l'assistant vanilla démarre directement au scan du monde (modèle Game Master imposé). Cliquez sur **Scan world**, **Create entities**, puis **Next** jusqu'au bout. Le mission header est créé au nom de l'opération.
7. **Éléments** : ils sont placés devant la caméra du World Editor, chacun dans son calque, et restent sélectionnés pour les déplacer d'un coup. Le monde est enregistré et le calque actif devient `Jour_1`. La dernière fenêtre liste les fichiers à modifier ; **Ouvrir le briefing** l'ouvre dans le Resource Manager.

Ce qui est déjà dans le monde n'est pas placé une deuxième fois : vous pouvez relancer le plugin sans risque.

## Calques du monde

| Calque | Contenu |
|---|---|
| `Composants_Mission` | composants système : mode de jeu Game Master et ses gestionnaires, AI World, Perception Manager, `CK_Troops`, `CK_MissionSettings` |
| `Base_Avancee` | la base avancée (FOB), d'où l'on part : arsenaux, dépôts de véhicules, point de spawn « Base avancée », estrade CK |
| `Jour_1` à `Jour_4` | vides : un calque par jour de l'opération (un vendredi = un jour) |

Le monde n'a pas de calque `default`. Si un monde en a déjà un (créé avec une ancienne version du plugin), le plugin range les entités du scénario dans leurs calques et supprime `default` quand il est vide. Les entités que vous avez mises dans un autre calque ne bougent pas.

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

| Élément | Calque | Mod | Prefab |
|---|---|---|---|
| Système des troops | `Composants_Mission` | CK Core | `Prefabs/CK/CK_Troops.et` |
| Infos de mission du menu CK (briefing, loadouts) | `Composants_Mission` | CK Core | `Prefabs/CK/CK_MissionSettings.et` |
| Estrade avec insigne CK | `Base_Avancee` | CK Props | `Prefabs/Props/Military/Camps/Platform_01_CK.et` |
| Dépôt de véhicules - héliport | `Base_Avancee` | CK Structures | `Prefabs/Structures/Military/Garages/CK_Vehicle_Depot/CK_Vehicle_Depot_Helipad.et` |
| Dépôt de véhicules - plateforme | `Base_Avancee` | CK Structures | `Prefabs/Structures/Military/Garages/CK_Vehicle_Depot/CK_Vehicle_Depot_Platform.et` |
| Arsenaux armes, munitions, médical, équipement | `Base_Avancee` | Arma Reforger | `Prefabs/Props/Military/Arsenal/AmmoBoxes/US/AmmoBoxArsenal_*_US.et` |
| Point de spawn « Base avancée » | `Base_Avancee` | Arma Reforger | `Prefabs/MP/Spawning/SpawnPoint_US.et` |

Point de spawn : en Game Master, les factions ne sont pas jouables au départ (le GM doit les activer). Le plugin rend la faction US jouable dans le gestionnaire de factions du mode de jeu, pour que les joueurs puissent se déployer sur « Base avancée » dès le début.

## Bon à savoir

- Game Mode Setup ne regarde que la sous-scène : sur un terrain qui a déjà une carte (Arland, Everon…), il en crée une deuxième (erreur « Multiple map entities present! » dans la console). Le plugin supprime ce doublon, ainsi que ceux de l'AI World et du Perception Manager si le terrain en a déjà.
- Le menu Plugins contient aussi **CK - Game Mode Setup (Game Master)** : c'est l'assistant vanilla sans le choix du modèle, utilisé par le plugin. Il peut servir seul.
- Après le renommage, le launcher Workbench garde l'ancien chemin du projet : ajoutez le nouveau dossier avec **Add Existing**.
- Avant de modifier `addon.gproj`, le plugin en garde une copie dans `Documents\My Games\ArmaReforgerWorkbench\profile\CK_ConfigurationScenario\` (avec l'état de la configuration en cours).
- Le dossier du plugin peut rester dans le projet publié : le jeu ne charge pas les scripts `WorkbenchGame`.
- Mise à jour : remplacez simplement le fichier `.c`.
- Testé avec Arma Reforger Tools 1.8.
