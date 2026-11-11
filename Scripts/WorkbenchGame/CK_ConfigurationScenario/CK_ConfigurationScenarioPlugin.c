#ifdef WORKBENCH
//! CK - Configuration Scénario
//! 1. Dépendances CK (et autres GUID) ajoutées au .gproj du projet.
//! 2. Terrain du scénario, nom, date, lieu et scénaristes de l'opération.
//! 3. Mod renommé CK_Op_<année>_<mois>_<jour> (mois et jour sur 2 chiffres : dossier, TITLE, ID), puis redémarrage de Workbench si besoin
//!    (dossier renommé et plugin relancé par un .bat, voir RestartWorkbench).
//! 4. Sous-scène Worlds/<mod>/<mod>.ent créée dans le mod et ouverte.
//! 5. Briefing, loadouts CK (tenue place d'armes et paquetage recrue), Game Mode Setup en Game Master (mission header : nom de l'opération,
//!    scénaristes, description et images CK).
//! 6. Calques Composants_Mission (systèmes), Base_Avancee (FOB) et Jour_1 à Jour_4, éléments CK, arsenaux et point de spawn
//!    US "Base avancée" placés, intro au nom et au lieu de l'opération, factions US et USSR jouables, monde enregistré.
//! Aucune classe CK n'est référencée par ce script : il compile sans les mods CK (avant l'ajout des dépendances).
[WorkbenchPluginAttribute(name: "CK - Configuration Scénario", description: "Dépendances CK, monde, mode de jeu Game Master et éléments CK du scénario", wbModules: { "ResourceManager", "WorldEditor" }, awesomeFontCode: 0xF024)]
class CK_ConfigurationScenarioPlugin : WorkbenchPlugin
{
	static const string TITLE = "CK - Configuration Scénario";
	static const string GUID_ARMA_REFORGER = "58D0FB3206B6F859";
	static const string PROFILE_DIR = "$profile:CK_ConfigurationScenario";
	static const string PLUGIN_FILE = "Scripts/WorkbenchGame/CK_ConfigurationScenario/CK_ConfigurationScenarioPlugin.c";
	static const string PLUGIN_CLASS = "CK_ConfigurationScenarioPlugin";
	static const string GAME_MASTER_TEMPLATE = "{6389BA4D41B187DC}Configs/Workbench/GameModeSetup/GameMaster.conf";
	static const string SPAWN_POINT_NAME = "Base avancée";
	static const string FACTION_KEY = "US";
	static const string LAYER_SYSTEMS = "Composants_Mission";
	static const string LAYER_BASE = "Base_Avancee";
	static const string LAYER_DAY = "Jour_";
	static const int DAY_COUNT = 4;
	static const string DEFAULT_LOADOUT_PLACE_DARMES = "Tenue place d'armes";
	static const string DEFAULT_LOADOUT_RECRUE = "Paquetage recrue";
	static const string OLD_OUTFIT_NAME = "Tenue de briefing";			//!< default loadout of the previous versions (replaced)
	static const string OLD_OUTFIT_PREFAB_SUFFIX = "_TenueBriefing.et";
	static const string MISSION_INTRO_PREFAB = "{1C31E7D9936A78A5}Prefabs/CK/CK_MissionIntro.et";

	//--- Mission header: description and pictures. Icon: CK Core logo (= thumbnail.png of the CK mods). Loading screen
	//--- and preview: textures shipped with the plugin (UI/Textures/CK_Scenario, copied into the scenario with Scripts).
	static const string HEADER_DESCRIPTION = "Un scénario du commando Kieffer";
	static const string HEADER_ICON = "{61C4A8F2E59B3D07}UI/Textures/CK/CK_Logo.edds";
	static const string HEADER_LOADING_SCREEN = "{1C741E00470F1914}UI/Textures/CK_Scenario/CK_MissionLoadingScreen.edds";
	static const string HEADER_PREVIEW_IMAGE = "{DBCF99A13ED844E8}UI/Textures/CK_Scenario/CK_MissionPreview.edds";

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		RunAll(false);
	}

	//------------------------------------------------------------------------------------------------
	//! Workbench started by RestartWorkbench() with -plugin=CK_ConfigurationScenarioPlugin: resume the configuration
	override void RunCommandline()
	{
		//--- In the log of the restart: this line missing means Workbench opened another project
		FileHandle log = FileIO.OpenFile(PROFILE_DIR + "/restart.log", FileMode.APPEND);
		if (log)
		{
			log.WriteLine("Workbench rouvert, plugin lancé dans le projet : " + FindProjectFile());
			log.Close();
		}

		RunAll(true);
	}

	//------------------------------------------------------------------------------------------------
	protected void RunAll(bool fromCommandLine)
	{
		string projectGuid = FindProjectAddon();
		if (projectGuid.IsEmpty())
		{
			if (!fromCommandLine)
				Workbench.Dialog(TITLE, "Le plugin doit être copié dans le projet du scénario (voir README).");

			return;
		}

		string gprojPath = FindAddonProjectFile(projectGuid);
		CK_ConfigurationScenarioProjectFile project = new CK_ConfigurationScenarioProjectFile();
		if (gprojPath.IsEmpty() || !project.Load(gprojPath))
		{
			Workbench.Dialog(TITLE, "Impossible de lire le fichier .gproj du projet.\nSi Workbench a demandé une autorisation de script, acceptez-la (redémarrez Workbench si vous l'avez refusée).");
			return;
		}

		CK_ConfigurationScenarioState state = CK_ConfigurationScenarioState.Load(projectGuid);
		if (!state)
		{
			state = new CK_ConfigurationScenarioState();
			state.m_sProjectGuid = projectGuid;
		}

		//--- Resume after the restart
		bool resumeOffered;
		if (state.m_bPending && IsProjectReady(project))
		{
			resumeOffered = true;
			CK_ConfigurationScenario_ResumeDialog resumeDialog = new CK_ConfigurationScenario_ResumeDialog();
			string resumeText = string.Format("Reprise de la configuration :\n - opération : %1\n - date : %2\n - lieu : %3\n - scénaristes : %4\n - mod : %5", state.m_sOperation, state.GetDateText(), state.GetLocation(), state.GetAuthors(), state.GetModName());
			if (!Workbench.ScriptDialog(TITLE, resumeText, resumeDialog))
				return;

			if (resumeDialog.m_iResult == CK_ConfigurationScenario_ResumeDialog.RESULT_CONTINUE)
			{
				RunSetup(project, state);
				return;
			}
		}

		//--- Started from the command line without anything to resume: nothing to do. Restarted by the plugin but the
		//--- project is not ready: say why instead of leaving an empty World Editor
		if (fromCommandLine && !resumeOffered)
		{
			if (state.m_bPending)
			{
				string reason = GetNotLoadedDependencies(project);
				if (!reason.IsEmpty())
					reason = "Dépendances non chargées :\n" + reason;

				string loadedId = GameProject.GetAddonID(project.GetGuid());
				if (loadedId != project.GetValue("ID"))
					reason += string.Format("ID du projet chargé : %1, ID du .gproj : %2\n", loadedId, project.GetValue("ID"));

				Workbench.Dialog(TITLE, "La configuration ne peut pas reprendre après le redémarrage.\n" + reason + "\nCorrigez ce qui manque, puis relancez le plugin et cliquez sur Continuer.");
			}

			return;
		}

		//--- 1. Dependencies, 2. terrain, 3. operation
		array<string> wanted = {};
		if (!RunDependencies(project, wanted))
			return;

		if (!RunWorldChoice(state))
			return;

		if (!RunOperation(state))
			return;

		//--- Project changes: dependencies, TITLE and ID
		string modName = state.GetModName();
		project.AddDependencies(wanted);
		project.SetValue("TITLE", modName);
		project.SetValue("ID", GetAddonId(modName));
		if (project.IsModified())
		{
			project.Backup(PROFILE_DIR);
			if (!project.Save())
			{
				Workbench.Dialog(TITLE, "Impossible d'écrire le fichier du projet :\n" + gprojPath);
				return;
			}

			Print(TITLE + ": projet mis à jour : " + gprojPath, LogLevel.NORMAL);
		}

		state.m_bPending = true;
		state.Save();

		//--- Restart when a dependency is not loaded, the ID changed or the folder must be renamed
		string restartText = GetRestartReasons(project, modName);
		if (!restartText.IsEmpty())
		{
			restartText += "\nEnregistrez votre travail avant de redémarrer. Le plugin reprend après le redémarrage (sinon, relancez-le).";

			CK_ConfigurationScenario_RestartDialog restartDialog = new CK_ConfigurationScenario_RestartDialog();
			if (!Workbench.ScriptDialog(TITLE, restartText, restartDialog))
				return;

			//--- Launcher project list: the renamed project (the old folder would be mounted at startup and fail)
			state.m_sListedPath = GetRenamedProjectPath(project, modName);
			state.Save();
			UpdateProjectLists(project.GetGuid(), state.m_sListedPath);

			if (!RestartWorkbench(gprojPath, modName))
				Workbench.Dialog(TITLE, "Redémarrage automatique impossible.\nFermez Workbench, rouvrez le projet puis relancez le plugin.");

			return;
		}

		RunSetup(project, state);
	}

	//------------------------------------------------------------------------------------------------
	// Project
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	//! Workbench.GetCurrentGameProjectFile() returns the Arma Reforger .gproj, not the opened mod:
	//! the scenario project is the loaded addon that contains this plugin (last one if several).
	//! Only path resolution here: files are read in the scenario project only.
	//! Note: the first file access of a Workbench session triggers Workbench's script authorisation prompt.
	//! \return GUID of that addon, empty if not found
	static string FindProjectAddon()
	{
		array<string> loaded = {};
		GameProject.GetLoadedAddons(loaded);

		for (int i = loaded.Count() - 1; i >= 0; i--)
		{
			string guid = loaded[i];
			if (GameProject.IsVanillaAddon(guid))
				continue;

			string fileSystem = SCR_AddonTool.ToFileSystem(GameProject.GetAddonID(guid));
			string pluginPath;
			if (!fileSystem.IsEmpty() && Workbench.GetAbsolutePath(fileSystem + PLUGIN_FILE, pluginPath, true))
				return guid;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! \return absolute path of the scenario project's .gproj, empty if not found
	static string FindProjectFile()
	{
		string guid = FindProjectAddon();
		if (guid.IsEmpty())
			return string.Empty;

		return FindAddonProjectFile(guid);
	}

	//------------------------------------------------------------------------------------------------
	//! \return absolute path of the .gproj at the root of a loaded addon whose GUID matches, empty if not found
	static string FindAddonProjectFile(string addonGuid)
	{
		string fileSystem = SCR_AddonTool.ToFileSystem(GameProject.GetAddonID(addonGuid));
		if (fileSystem.IsEmpty())
			return string.Empty;

		string guid = addonGuid;
		guid.ToUpper();

		array<string> files = { fileSystem + "addon.gproj" };
		files.InsertAll(SCR_FileIOHelper.FindFiles(fileSystem, ".gproj"));
		foreach (string file : files)
		{
			string absolutePath;
			if (!Workbench.GetAbsolutePath(file, absolutePath, true))
				continue;

			absolutePath = CleanPath(absolutePath);
			CK_ConfigurationScenarioProjectFile project = new CK_ConfigurationScenarioProjectFile();
			if (project.Load(absolutePath) && project.GetGuid() == guid)
				return absolutePath;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! File system of the scenario project, e.g. "$CKOp20261202:" (the ID loaded by Workbench, not the one in the .gproj)
	static string GetProjectFileSystem(string projectGuid)
	{
		return SCR_AddonTool.ToFileSystem(GameProject.GetAddonID(projectGuid));
	}

	//------------------------------------------------------------------------------------------------
	//! Addon ID the way Workbench builds it from a name: letters and digits only ("CK_Op_2026_12_02" -> "CKOp20261202")
	static string GetAddonId(string name)
	{
		string allowed = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";
		string result;
		for (int i = 0, length = name.Length(); i < length; i++)
		{
			string c = name.Get(i);
			if (allowed.Contains(c))
				result += c;
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Ready to resume: every dependency loaded and the ID of the .gproj is the one Workbench loaded
	static bool IsProjectReady(notnull CK_ConfigurationScenarioProjectFile project)
	{
		return GetNotLoadedDependencies(project).IsEmpty() && GameProject.GetAddonID(project.GetGuid()) == project.GetValue("ID");
	}

	//------------------------------------------------------------------------------------------------
	//! \return " - Name (GUID)" lines of the dependencies of the project that Workbench did not load
	static string GetNotLoadedDependencies(notnull CK_ConfigurationScenarioProjectFile project)
	{
		array<string> loaded = {};
		GameProject.GetLoadedAddons(loaded);
		ToUpperAll(loaded);

		array<string> dependencies = {};
		project.GetDependencies(dependencies);

		string result;
		foreach (string dependencyGuid : dependencies)
		{
			if (loaded.Contains(dependencyGuid))
				continue;

			result += string.Format(" - %1", GetAddonLabel(dependencyGuid));
			if (GameProject.GetAddonTitle(dependencyGuid).IsEmpty())
				result += " : introuvable, à télécharger ou à ajouter au launcher Workbench";

			result += "\n";
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! \return text of the restart dialog, empty when no restart is needed
	static string GetRestartReasons(notnull CK_ConfigurationScenarioProjectFile project, string modName)
	{
		string reasons;
		string info;
		string folderName = GetFolderName(project.GetPath());
		CK_EConfigurationScenarioFolder folderState = GetFolderState(project, modName);
		if (folderState == CK_EConfigurationScenarioFolder.TO_RENAME)
			reasons += string.Format(" - dossier du mod renommé : %1 -> %2\n", folderName, modName);
		else if (folderState == CK_EConfigurationScenarioFolder.TAKEN)
			info += string.Format("\nLe dossier %1 existe déjà (autre projet) : le dossier %2 garde son nom.\n", modName, folderName);

		if (GameProject.GetAddonID(project.GetGuid()) != project.GetValue("ID"))
			reasons += string.Format(" - nom du mod : %1 (ID %2)\n", modName, project.GetValue("ID"));

		string notLoaded = GetNotLoadedDependencies(project);
		if (!notLoaded.IsEmpty())
			reasons += " - dépendances à charger :\n" + notLoaded;

		if (reasons.IsEmpty())
			return string.Empty;

		return "Workbench doit redémarrer :\n" + reasons + info;
	}

	//------------------------------------------------------------------------------------------------
	//! .gproj path once the folder is renamed to modName (current path when no rename is planned)
	static string GetRenamedProjectPath(notnull CK_ConfigurationScenarioProjectFile project, string modName)
	{
		string projectPath = CleanPath(project.GetPath());
		if (GetFolderState(project, modName) != CK_EConfigurationScenarioFolder.TO_RENAME)
			return projectPath;

		string projectDirectory = GetDirectory(projectPath);
		string parentDirectory = GetDirectory(projectDirectory.Substring(0, projectDirectory.Length() - 1));
		return parentDirectory + modName + "/" + FilePath.StripPath(projectPath);
	}

	//------------------------------------------------------------------------------------------------
	//! Is the project folder named modName? Workbench may give a short 8.3 path ("CK_OP_~1"): the folder named
	//! modName next to it is then checked, by the GUID of its .gproj.
	static CK_EConfigurationScenarioFolder GetFolderState(notnull CK_ConfigurationScenarioProjectFile project, string modName)
	{
		string projectPath = CleanPath(project.GetPath());
		if (GetFolderName(projectPath) == modName)
			return CK_EConfigurationScenarioFolder.RENAMED;

		string projectDirectory = GetDirectory(projectPath);
		string parentDirectory = GetDirectory(projectDirectory.Substring(0, projectDirectory.Length() - 1));
		string target = parentDirectory + modName;
		if (!FileIO.FileExists(target))
			return CK_EConfigurationScenarioFolder.TO_RENAME;

		CK_ConfigurationScenarioProjectFile other = new CK_ConfigurationScenarioProjectFile();
		if (other.Load(target + "/" + FilePath.StripPath(projectPath)) && other.GetGuid() == project.GetGuid())
			return CK_EConfigurationScenarioFolder.RENAMED;

		return CK_EConfigurationScenarioFolder.TAKEN;
	}

	//------------------------------------------------------------------------------------------------
	//! Folder of a file: "C:/a/CK_Op_2026_12_02/addon.gproj" -> "CK_Op_2026_12_02"
	static string GetFolderName(string filePath)
	{
		string directory = GetDirectory(filePath);
		if (directory.EndsWith("/"))
			directory = directory.Substring(0, directory.Length() - 1);

		return FilePath.StripPath(directory);
	}

	//------------------------------------------------------------------------------------------------
	//! "C:/a/b/file.ext" -> "C:/a/b/"
	static string GetDirectory(string filePath)
	{
		string path = CleanPath(filePath);
		return path.Substring(0, path.Length() - FilePath.StripPath(path).Length());
	}

	//------------------------------------------------------------------------------------------------
	//! Forward slashes, no doubled separator (except a leading "//" network path)
	static string CleanPath(string path)
	{
		string result = path;
		result.Replace("\\", "/");
		if (result.Length() < 3)
			return result;

		string head = result.Substring(0, 2);
		string tail = result.Substring(2, result.Length() - 2);
		int replaced = tail.Replace("//", "/");
		while (replaced > 0)
		{
			replaced = tail.Replace("//", "/");
		}

		return head + tail;
	}

	//------------------------------------------------------------------------------------------------
	static string ToWindowsPath(string path)
	{
		string result = path;
		result.Replace("/", "\\");
		return result;
	}

	//------------------------------------------------------------------------------------------------
	static void ToUpperAll(notnull array<string> values)
	{
		for (int i = 0, count = values.Count(); i < count; i++)
		{
			string value = values[i];
			value.ToUpper();
			values[i] = value;
		}
	}

	//------------------------------------------------------------------------------------------------
	// 1. Dépendances
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	static void GetDefaultDependencies(notnull array<string> guids, notnull array<string> names)
	{
		guids.Insert("6A9E08A3BF66AFF4");
		names.Insert("CK Core");
		guids.Insert("6A8CDBAAAC085EFF");
		names.Insert("CK Props");
		guids.Insert("6AA514EDBA6B2C30");
		names.Insert("CK Structures");
	}

	//------------------------------------------------------------------------------------------------
	//! Dialogs only: the dependencies are written with the other project changes
	//! \param[out] wanted CK dependencies and extra GUIDs
	//! \return false when cancelled
	protected bool RunDependencies(notnull CK_ConfigurationScenarioProjectFile project, notnull array<string> wanted)
	{
		string projectGuid = project.GetGuid();

		array<string> defaultGuids = {};
		array<string> defaultNames = {};
		GetDefaultDependencies(defaultGuids, defaultNames);

		string text = "Dépendances CK du projet :\n";
		foreach (int i, string guid : defaultGuids)
		{
			if (guid == projectGuid)
				continue;

			wanted.Insert(guid);
			text += string.Format(" - %1 (%2)", defaultNames[i], guid);
			if (project.HasDependency(guid))
				text += " : déjà présente";

			text += "\n";
		}

		array<string> current = {};
		project.GetDependencies(current);
		string others;
		foreach (string currentGuid : current)
		{
			if (currentGuid != GUID_ARMA_REFORGER && !defaultGuids.Contains(currentGuid))
				others += string.Format(" - %1\n", GetAddonLabel(currentGuid));
		}

		if (!others.IsEmpty())
			text += "\nAutres dépendances du projet :\n" + others;

		text += "\nAjouter d'autres dépendances ?";

		CK_ConfigurationScenario_DependenciesDialog dependenciesDialog = new CK_ConfigurationScenario_DependenciesDialog();
		if (!Workbench.ScriptDialog(TITLE, text, dependenciesDialog))
			return false;

		if (dependenciesDialog.m_iResult != CK_ConfigurationScenario_DependenciesDialog.RESULT_ADD)
			return true;

		CK_ConfigurationScenario_GuidDialog guidDialog = new CK_ConfigurationScenario_GuidDialog();
		while (true)
		{
			if (!Workbench.ScriptDialog(TITLE, "GUID des autres mods : un par ligne, ou séparés par des virgules.", guidDialog))
				return false;

			array<string> valid = {};
			array<string> invalid = {};
			ParseGuidList(guidDialog.m_sGUID, valid, invalid);
			if (!invalid.IsEmpty())
			{
				string invalidText = "GUID non valides (16 caractères 0-9 / A-F) :\n";
				foreach (string token : invalid)
				{
					invalidText += " - " + token + "\n";
				}

				Workbench.Dialog(TITLE, invalidText);
				continue;
			}

			foreach (string validGuid : valid)
			{
				if (validGuid != projectGuid && !wanted.Contains(validGuid))
					wanted.Insert(validGuid);
			}

			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! "Name (GUID)" when Workbench knows the addon, else the GUID
	static string GetAddonLabel(string guid)
	{
		array<string> defaultGuids = {};
		array<string> defaultNames = {};
		GetDefaultDependencies(defaultGuids, defaultNames);
		int index = defaultGuids.Find(guid);
		if (index >= 0)
			return string.Format("%1 (%2)", defaultNames[index], guid);

		string title = GameProject.GetAddonTitle(guid);
		if (title.IsEmpty())
			return guid;

		return string.Format("%1 (%2)", title, guid);
	}

	//------------------------------------------------------------------------------------------------
	//! Split a user list of GUIDs (commas, semicolons, spaces, line breaks, {braces} and quotes allowed)
	static void ParseGuidList(string text, notnull array<string> valid, notnull array<string> invalid)
	{
		int asciiCR = 13;
		int asciiLF = 10;
		int asciiTab = 9;

		string clean = text;
		clean.Replace(asciiCR.AsciiToString(), " ");
		clean.Replace(asciiLF.AsciiToString(), " ");
		clean.Replace(asciiTab.AsciiToString(), " ");
		clean.Replace(",", " ");
		clean.Replace(";", " ");
		clean.Replace("{", " ");
		clean.Replace("}", " ");
		clean.Replace("\"", " ");

		array<string> tokens = {};
		clean.Split(" ", tokens, true);
		foreach (string token : tokens)
		{
			string guid = token.Trim();
			if (guid.IsEmpty())
				continue;

			guid.ToUpper();
			if (!IsGuid(guid))
			{
				invalid.Insert(token);
				continue;
			}

			if (!valid.Contains(guid))
				valid.Insert(guid);
		}
	}

	//------------------------------------------------------------------------------------------------
	static bool IsGuid(string text)
	{
		if (text.Length() != 16)
			return false;

		string hex = "0123456789ABCDEF";
		for (int i = 0; i < 16; i++)
		{
			if (!hex.Contains(text.Get(i)))
				return false;
		}

		return true;
	}

	//------------------------------------------------------------------------------------------------
	// 2. Terrain, 3. opération
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected bool RunWorldChoice(notnull CK_ConfigurationScenarioState state)
	{
		CK_ConfigurationScenario_WorldDialog worldDialog = new CK_ConfigurationScenario_WorldDialog();
		string world = state.m_sParentWorld;
		if (world.IsEmpty())
			world = GetCurrentWorld();

		if (!GetResourceGuid(world).IsEmpty())
			worldDialog.m_sMonde = world;

		while (true)
		{
			if (!Workbench.ScriptDialog(TITLE, "Terrain du scénario (monde .ent, par ex. Everon ou Arland).\nLe plugin crée la sous-scène du scénario dans le mod.", worldDialog))
				return false;

			if (!worldDialog.m_sMonde.IsEmpty())
				break;

			Workbench.Dialog(TITLE, "Aucun monde choisi.");
		}

		state.m_sParentWorld = worldDialog.m_sMonde;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	protected bool RunOperation(notnull CK_ConfigurationScenarioState state)
	{
		CK_ConfigurationScenario_OperationDialog operationDialog = new CK_ConfigurationScenario_OperationDialog();
		operationDialog.m_sNom = state.m_sOperation;
		operationDialog.m_sLieu = state.m_sLocation;
		operationDialog.m_sScenaristes = state.m_sAuthors;
		if (state.HasDate())
		{
			operationDialog.m_iDate = PackDate(state.m_iYear, state.m_iMonth, state.m_iDay);
		}
		else
		{
			int year, month, day;
			System.GetYearMonthDay(year, month, day);
			operationDialog.m_iDate = PackDate(year, month, day);
		}

		while (true)
		{
			if (!Workbench.ScriptDialog(TITLE, "L'opération : nom, date, lieu et scénaristes.", operationDialog))
				return false;

			string operation = operationDialog.m_sNom.Trim();
			string location = operationDialog.m_sLieu.Trim();
			string authors = CleanAuthors(operationDialog.m_sScenaristes);
			int pickedYear, pickedMonth, pickedDay;
			UnpackDate(operationDialog.m_iDate, pickedYear, pickedMonth, pickedDay);

			if (operation.IsEmpty())
			{
				Workbench.Dialog(TITLE, "Donnez un nom à l'opération.");
				continue;
			}

			if (!IsValidDate(pickedYear, pickedMonth, pickedDay))
			{
				Workbench.Dialog(TITLE, "Date non valide.");
				continue;
			}

			CK_ConfigurationScenarioState preview = new CK_ConfigurationScenarioState();
			preview.m_iYear = pickedYear;
			preview.m_iMonth = pickedMonth;
			preview.m_iDay = pickedDay;
			string modName = preview.GetModName();

			preview.m_sLocation = location;
			preview.m_sAuthors = authors;
			string confirmText = string.Format("Opération : %1\nDate : %2\nLieu : %3\nScénaristes : %4\n\nMod : %5\nMonde : Worlds/%5/%5.ent", operation, preview.GetDateText(), preview.GetLocation(), preview.GetAuthors(), modName);
			CK_ConfigurationScenario_ConfirmDialog confirmDialog = new CK_ConfigurationScenario_ConfirmDialog();
			if (!Workbench.ScriptDialog(TITLE, confirmText, confirmDialog))
				return false;

			if (confirmDialog.m_iResult == CK_ConfigurationScenario_ConfirmDialog.RESULT_EDIT)
				continue;

			state.m_sOperation = operation;
			state.m_iYear = pickedYear;
			state.m_iMonth = pickedMonth;
			state.m_iDay = pickedDay;
			state.m_sLocation = location;
			state.m_sAuthors = authors;
			return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Workbench launcher project lists ($profile:.projectList_<app>_<user>.conf): the entry of this project points to path,
	//! added first when the project is not listed. Its entry is found by the GUID of the .gproj, or by previousPath (path
	//! written before a restart whose rename failed). Workbench mounts every listed project at startup and only writes
	//! these files from the launcher (new / open / add project), not when it closes.
	static void UpdateProjectLists(string projectGuid, string path, string previousPath = string.Empty)
	{
		array<string> files = SCR_FileIOHelper.FindFiles("$profile:", ".conf");
		foreach (string file : files)
		{
			//--- "$profile:.projectList_app1874910_user7656....conf" (no folder separator after the file system name)
			if (FilePath.StripPath(file).Contains(".projectList_"))
				UpdateProjectList(file, projectGuid, path, previousPath);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! \return true when the list file was changed
	static bool UpdateProjectList(string listFile, string projectGuid, string path, string previousPath)
	{
		FileHandle input = FileIO.OpenFile(listFile, FileMode.READ);
		if (!input)
			return false;

		array<string> lines = {};
		string line;
		while (input.ReadLine(line) >= 0)
		{
			int length = line.Length();
			if (length > 0 && line.ToAscii(length - 1) == 13)
				line = line.Substring(0, length - 1);

			lines.Insert(line);
		}

		input.Close();

		path.Replace("\\", "/");
		string target = NormalizeListPath(path);
		string previous = NormalizeListPath(previousPath);
		int projectsLine = -1;
		bool listed;
		bool changed;
		for (int i = 0; i < lines.Count(); i++)
		{
			string trimmed = lines[i].Trim();
			if (projectsLine < 0 && trimmed.StartsWith("Projects {"))
				projectsLine = i;

			if (!trimmed.StartsWith("FilePath \"") || !trimmed.EndsWith("\""))
				continue;

			string entry = trimmed.Substring(10, trimmed.Length() - 11);
			string normalized = NormalizeListPath(entry);
			bool isProject = normalized == target || (!previous.IsEmpty() && normalized == previous);
			if (!isProject && FileIO.FileExists(entry))
			{
				CK_ConfigurationScenarioProjectFile other = new CK_ConfigurationScenarioProjectFile();
				isProject = other.Load(entry) && other.GetGuid() == projectGuid;
			}

			if (!isProject)
				continue;

			//--- Second entry of the project: removed (WBProjectListItem { FilePath } block)
			if (listed && i > 0 && i + 1 < lines.Count() && lines[i - 1].Trim() == "WBProjectListItem {" && lines[i + 1].Trim() == "}")
			{
				lines.RemoveOrdered(i + 1);
				lines.RemoveOrdered(i);
				lines.RemoveOrdered(i - 1);
				i -= 2;
				changed = true;
				continue;
			}

			listed = true;
			if (entry != path)
			{
				int indent = lines[i].IndexOf("FilePath");
				lines[i] = lines[i].Substring(0, indent) + "FilePath \"" + path + "\"";
				changed = true;
			}
		}

		if (!listed)
		{
			if (projectsLine < 0)
				return false;

			lines.InsertAt("  }", projectsLine + 1);
			lines.InsertAt("   FilePath \"" + path + "\"", projectsLine + 1);
			lines.InsertAt("  WBProjectListItem {", projectsLine + 1);
			changed = true;
		}

		if (!changed)
			return false;

		FileHandle output = FileIO.OpenFile(listFile, FileMode.WRITE);
		if (!output)
			return false;

		foreach (string outputLine : lines)
		{
			output.WriteLine(outputLine);
		}

		output.Close();
		Print(TITLE + ": projet dans la liste du launcher Workbench : " + path, LogLevel.NORMAL);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	static string NormalizeListPath(string path)
	{
		path.Replace("\\", "/");
		path.ToLower();
		return path;
	}

	//------------------------------------------------------------------------------------------------
	//! "Jean ; Paul,, Marc" -> "Jean, Paul, Marc" (separators: comma, semicolon, slash, line break)
	static string CleanAuthors(string text)
	{
		text.Replace(";", ",");
		text.Replace("/", ",");
		text.Replace("\n", ",");
		array<string> parts = {};
		text.Split(",", parts, true);
		string result;
		foreach (string part : parts)
		{
			string author = part.Trim();
			if (author.IsEmpty())
				continue;

			if (!result.IsEmpty())
				result += ", ";

			result += author;
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! UIWidgets.Date value (DateTimeAsInt): sec 6 bits, min 6, hour 5, day 5, month 4, year - 2000 above (noon here)
	static int PackDate(int year, int month, int day)
	{
		return ((year - 2000) << 26) | (month << 22) | (day << 17) | (12 << 12);
	}

	//------------------------------------------------------------------------------------------------
	static void UnpackDate(int packed, out int year, out int month, out int day)
	{
		year = 2000 + ((packed >> 26) & 63);
		month = (packed >> 22) & 15;
		day = (packed >> 17) & 31;
	}

	//------------------------------------------------------------------------------------------------
	static bool IsValidDate(int year, int month, int day)
	{
		if (year < 2000 || month < 1 || month > 12 || day < 1)
			return false;

		int days = 31;
		if (month == 4 || month == 6 || month == 9 || month == 11)
			days = 30;

		if (month == 2)
		{
			days = 28;
			if ((year % 4 == 0 && year % 100 != 0) || year % 400 == 0)
				days = 29;
		}

		return day <= days;
	}

	//------------------------------------------------------------------------------------------------
	static string GetMonthName(int month)
	{
		array<string> names = { "janvier", "février", "mars", "avril", "mai", "juin", "juillet", "août", "septembre", "octobre", "novembre", "décembre" };
		if (month < 1 || month > 12)
			return month.ToString();

		return names[month - 1];
	}

	//------------------------------------------------------------------------------------------------
	//! Start a batch file that waits for this Workbench to close, renames the mod folder,
	//! then opens the project again and starts this plugin (RunCommandline), then exit.
	//! \return false when the restart could not be prepared (Workbench keeps running)
	static bool RestartWorkbench(string gprojPath, string newFolderName, string pluginClass = "CK_ConfigurationScenarioPlugin")
	{
		string batchAbsPath = WriteRestartBatch(gprojPath, newFolderName, pluginClass);
		if (batchAbsPath.IsEmpty())
			return false;

		Print(TITLE + ": redémarrage de Workbench (" + batchAbsPath + ")", LogLevel.NORMAL);
		Workbench.RunCmd("cmd /c \"\"" + ToWindowsPath(batchAbsPath) + "\"\"", false);
		Workbench.Exit(0);
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Batch file of RestartWorkbench(), in the profile folder
	//! \return its absolute path, empty on failure
	static string WriteRestartBatch(string gprojPath, string newFolderName, string pluginClass)
	{
		string exePath;
		Workbench.GetExecutableFilePath(exePath);
		if (exePath.IsEmpty() || gprojPath.IsEmpty())
			return string.Empty;

		string gameDirectory = GetGameDirectory(exePath);
		string oldDirectory = ToWindowsPath(GetDirectory(gprojPath));
		if (oldDirectory.EndsWith("\\"))
			oldDirectory = oldDirectory.Substring(0, oldDirectory.Length() - 1);

		string parentDirectory = oldDirectory.Substring(0, oldDirectory.Length() - FilePath.StripPath(CleanPath(oldDirectory)).Length());
		string newDirectory = parentDirectory + newFolderName;
		string gprojName = FilePath.StripPath(CleanPath(gprojPath));

		string arguments = "-gproj \"%GPROJ%\"";
		string addonsDirectories = GetAddonsDirectories(gameDirectory, gprojPath);
		if (!addonsDirectories.IsEmpty())
			arguments += string.Format(" -addonsDir \"%1\"", addonsDirectories);

		string profileArgument;
		string value;
		if (System.GetCLIParam("profile", value) && !value.IsEmpty())
			profileArgument = string.Format(" -profile \"%1\"", value);

		arguments += profileArgument;

		if (!pluginClass.IsEmpty())
			arguments += " -wbModule=WorldEditor -plugin=" + pluginClass;

		string exeName = FilePath.StripPath(ToWindowsPath(exePath));

		FileIO.MakeDirectory(PROFILE_DIR);
		string profileDirectory;
		if (!Workbench.GetAbsolutePath(PROFILE_DIR, profileDirectory, true))
			return string.Empty;

		string logPath = ToWindowsPath(CleanPath(profileDirectory) + "/restart.log");
		string batchPath = PROFILE_DIR + "/restart.bat";
		FileHandle file = FileIO.OpenFile(batchPath, FileMode.WRITE);
		if (!file)
			return string.Empty;

		//--- Wait for Workbench to close, without time limit (it may ask to save first). CSV output: the table output of
		//--- tasklist cuts image names at 25 characters ("ArmaReforgerWorkbenchStea"), the old wait never matched.
		//--- The rename is retried: an antivirus, OneDrive, the Windows indexer or an Explorer window can hold the folder for a while.
		//--- The project is reopened where its .gproj really is: the old folder while it still exists, else the new one.
		file.WriteLine("@echo off");
		file.WriteLine("chcp 65001 >nul");
		file.WriteLine("set \"LOG=" + logPath + "\"");
		file.WriteLine("echo [%date% %time%] Redemarrage de Workbench > \"%LOG%\"");
		file.WriteLine(":wait");
		file.WriteLine("tasklist /fi \"imagename eq " + exeName + "\" /fo csv /nh 2>nul | find /i \"" + exeName + "\" >nul || goto closed");
		file.WriteLine("ping -n 2 127.0.0.1 >nul");
		file.WriteLine("goto wait");
		file.WriteLine(":closed");
		file.WriteLine("ping -n 3 127.0.0.1 >nul");
		file.WriteLine("echo [%time%] Workbench ferme >> \"%LOG%\"");
		file.WriteLine("set \"OLD=" + oldDirectory + "\"");
		file.WriteLine("set \"NEW=" + newDirectory + "\"");
		file.WriteLine("set \"GPROJNAME=" + gprojName + "\"");
		file.WriteLine("echo ancien dossier : %OLD% >> \"%LOG%\"");
		file.WriteLine("echo nouveau dossier : %NEW% >> \"%LOG%\"");
		file.WriteLine("if /i \"%OLD%\"==\"%NEW%\" goto open");
		file.WriteLine("if exist \"%NEW%\\\" goto open");
		file.WriteLine("set /a TRY=0");
		file.WriteLine(":rename");
		file.WriteLine("ren \"%OLD%\" \"" + newFolderName + "\" >> \"%LOG%\" 2>&1 && goto renamed");
		file.WriteLine("set /a TRY+=1");
		file.WriteLine("if %TRY% geq 30 goto open");
		file.WriteLine("ping -n 2 127.0.0.1 >nul");
		file.WriteLine("goto rename");
		file.WriteLine(":renamed");
		file.WriteLine("echo [%time%] dossier renomme >> \"%LOG%\"");
		file.WriteLine(":open");
		file.WriteLine("set \"GPROJ=\"");
		file.WriteLine("if exist \"%OLD%\\%GPROJNAME%\" set \"GPROJ=%OLD%\\%GPROJNAME%\"");
		file.WriteLine("if not defined GPROJ if exist \"%NEW%\\%GPROJNAME%\" set \"GPROJ=%NEW%\\%GPROJNAME%\"");
		file.WriteLine("if not defined GPROJ echo ERREUR : %GPROJNAME% introuvable >> \"%LOG%\"");
		file.WriteLine("if not defined GPROJ goto launcher");
		file.WriteLine("echo projet ouvert : %GPROJ% >> \"%LOG%\"");

		//--- Without the game folder, Workbench started with -gproj opens an empty project:
		//--- open the launcher instead, the renamed project is first in its list
		if (gameDirectory.IsEmpty())
		{
			file.WriteLine("echo ERREUR : dossier d'Arma Reforger introuvable >> \"%LOG%\"");
			file.WriteLine("goto launcher");
		}
		else
		{
			file.WriteLine("cd /d \"" + ToWindowsPath(gameDirectory) + "\"");
			file.WriteLine("echo dossier du jeu : %CD% >> \"%LOG%\"");
			file.WriteLine("echo commande : \"" + ToWindowsPath(exePath) + "\" " + arguments + " >> \"%LOG%\"");
			file.WriteLine("start \"\" \"" + ToWindowsPath(exePath) + "\" " + arguments);
			file.WriteLine("goto end");
		}

		file.WriteLine(":launcher");
		file.WriteLine("echo ouverture du lanceur de Workbench >> \"%LOG%\"");
		file.WriteLine("cd /d \"" + ToWindowsPath(GetDirectory(exePath)) + "\"");
		file.WriteLine("start \"\" \"" + ToWindowsPath(exePath) + "\"" + profileArgument);
		file.WriteLine(":end");
		file.Close();

		string batchAbsPath;
		if (!Workbench.GetAbsolutePath(batchPath, batchAbsPath, true))
			return string.Empty;

		return batchAbsPath;
	}

	//------------------------------------------------------------------------------------------------
	//! Arma Reforger folder (contains addons/data/ArmaReforger.gproj). Workbench started from its launcher runs in
	//! "Arma Reforger Tools/Workbench": started with -gproj from there, it does not find the game.
	static string GetGameDirectory(string exePath)
	{
		string cwd;
		Workbench.GetCwd(cwd);
		cwd = CleanPath(cwd);
		if (cwd.EndsWith("/"))
			cwd = cwd.Substring(0, cwd.Length() - 1);

		array<string> candidates = {};

		//--- From the game project file ("./addons/data/ArmaReforger.gproj" relative to the working directory, or absolute)
		string gameProject = CleanPath(Workbench.GetCurrentGameProjectFile());
		if (gameProject.StartsWith("./"))
			gameProject = cwd + "/" + gameProject.Substring(2, gameProject.Length() - 2);

		int index = gameProject.LastIndexOf("/addons/");
		if (index > 0)
			candidates.Insert(gameProject.Substring(0, index));

		candidates.Insert(cwd);

		//--- Steam: ".../common/Arma Reforger Tools/Workbench/exe" -> ".../common/Arma Reforger"
		string exeDirectory = GetDirectory(exePath);
		int toolsIndex = exeDirectory.LastIndexOf("Arma Reforger Tools/");
		if (toolsIndex > 0)
			candidates.Insert(exeDirectory.Substring(0, toolsIndex) + "Arma Reforger");

		foreach (string candidate : candidates)
		{
			if (FileIO.FileExists(candidate + "/addons/data/ArmaReforger.gproj"))
				return candidate;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! Folders for -addonsDir, comma separated. A project opened with -gproj only sees these folders (the launcher list
	//! is not used): a dependency missing from them makes Workbench open the ArmaReforger project instead.
	//! Workbench takes the first mod found for a GUID and puts its default projects folder ("My Games/ArmaReforgerWorkbench/addons")
	//! after the folders given here. So projects are given one folder each, before the Workshop downloads
	//! ("My Games/ArmaReforger/addons", packed mods): a mod both in a project and downloaded comes from the project, as in the launcher.
	//! \param gprojPath project reopened by the restart (given by -gproj, its own folder is not added)
	static string GetAddonsDirectories(string gameDirectory, string gprojPath = string.Empty)
	{
		array<string> directories = {};
		if (!gameDirectory.IsEmpty())
			AddDirectory(directories, gameDirectory + "/addons");

		CK_ConfigurationScenarioProjectFile project = new CK_ConfigurationScenarioProjectFile();
		array<string> needed = {};
		if (!gprojPath.IsEmpty() && project.Load(gprojPath))
			project.GetDependencies(needed);

		//--- Loaded projects (a packed mod has no folder readable from script)
		array<string> loaded = {};
		array<string> found = {};
		GameProject.GetLoadedAddons(loaded);
		foreach (string guid : loaded)
		{
			if (guid == project.GetGuid() || GameProject.IsVanillaAddon(guid))
				continue;

			string root;
			if (Workbench.GetAbsolutePath(SCR_AddonTool.ToFileSystem(GameProject.GetAddonID(guid)), root, true))
			{
				AddDirectory(directories, root);
				found.Insert(guid);
			}
		}

		//--- Other dependencies (not loaded yet, or packed): project of the launcher list with that GUID, then its own dependencies
		array<string> listedFiles = {};
		array<string> listedGuids = {};
		GetListedProjects(listedFiles, listedGuids);
		for (int n = 0; n < needed.Count(); n++)
		{
			int listedIndex = listedGuids.Find(needed[n]);
			if (found.Contains(needed[n]) || listedIndex < 0 || needed[n] == project.GetGuid())
				continue;

			found.Insert(needed[n]);

			AddDirectory(directories, GetDirectory(listedFiles[listedIndex]));
			CK_ConfigurationScenarioProjectFile dependency = new CK_ConfigurationScenarioProjectFile();
			array<string> dependencyGuids = {};
			if (dependency.Load(listedFiles[listedIndex]))
				dependency.GetDependencies(dependencyGuids);

			foreach (string dependencyGuid : dependencyGuids)
			{
				if (!needed.Contains(dependencyGuid))
					needed.Insert(dependencyGuid);
			}
		}

		//--- Default projects folder, then Workshop downloads: next to the profile folder ("My Games/ArmaReforgerWorkbench/profile")
		string profile;
		if (Workbench.GetAbsolutePath("$profile:", profile, true))
		{
			profile = CleanPath(profile);
			if (profile.EndsWith("/"))
				profile = profile.Substring(0, profile.Length() - 1);

			string workbenchDirectory = GetDirectory(profile);
			AddDirectory(directories, workbenchDirectory + "addons");

			workbenchDirectory = workbenchDirectory.Substring(0, workbenchDirectory.Length() - 1);
			AddDirectory(directories, GetDirectory(workbenchDirectory) + "ArmaReforger/addons");
		}

		string result;
		foreach (int d, string directory : directories)
		{
			if (d > 0)
				result += ",";

			result += directory;
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! Add an existing folder once (Windows path, no trailing separator)
	static void AddDirectory(notnull array<string> directories, string path)
	{
		path = ToWindowsPath(CleanPath(path));
		while (path.EndsWith("\\"))
		{
			path = path.Substring(0, path.Length() - 1);
		}

		if (path.IsEmpty() || !FileIO.FileExists(path))
			return;

		string lowerPath = path;
		lowerPath.ToLower();
		foreach (string directory : directories)
		{
			string lowerDirectory = directory;
			lowerDirectory.ToLower();
			if (lowerDirectory == lowerPath)
				return;
		}

		directories.Insert(path);
	}

	//------------------------------------------------------------------------------------------------
	//! .gproj files of the launcher project lists that exist, with their GUID
	static void GetListedProjects(notnull array<string> files, notnull array<string> guids)
	{
		array<string> listFiles = SCR_FileIOHelper.FindFiles("$profile:", ".conf");
		foreach (string listFile : listFiles)
		{
			if (!FilePath.StripPath(listFile).Contains(".projectList_"))
				continue;

			FileHandle input = FileIO.OpenFile(listFile, FileMode.READ);
			if (!input)
				continue;

			string line;
			while (input.ReadLine(line) >= 0)
			{
				int length = line.Length();
				if (length > 0 && line.ToAscii(length - 1) == 13)
					line = line.Substring(0, length - 1);

				line = line.Trim();
				if (!line.StartsWith("FilePath \"") || !line.EndsWith("\""))
					continue;

				string entry = line.Substring(10, line.Length() - 11);
				CK_ConfigurationScenarioProjectFile listed = new CK_ConfigurationScenarioProjectFile();
				if (files.Contains(entry) || !FileIO.FileExists(entry) || !listed.Load(entry) || listed.GetGuid().IsEmpty())
					continue;

				files.Insert(entry);
				guids.Insert(listed.GetGuid());
			}

			input.Close();
		}
	}

	//------------------------------------------------------------------------------------------------
	// 4-6. Sous-scène, configs, Game Mode Setup, éléments
	//------------------------------------------------------------------------------------------------

	//------------------------------------------------------------------------------------------------
	protected void RunSetup(notnull CK_ConfigurationScenarioProjectFile project, notnull CK_ConfigurationScenarioState state)
	{
		string modName = state.GetModName();
		string fileSystem = GetProjectFileSystem(state.m_sProjectGuid);
		string notes;

		//--- Launcher project list: where the project really is now (also fixes the entry when the rename failed)
		UpdateProjectLists(project.GetGuid(), CleanPath(project.GetPath()), state.m_sListedPath);

		CK_EConfigurationScenarioFolder folderState = GetFolderState(project, modName);
		if (folderState == CK_EConfigurationScenarioFolder.TAKEN)
			notes += string.Format("Le dossier %1 existe déjà (autre projet) : le dossier du mod garde son nom.\n", modName);
		else if (folderState == CK_EConfigurationScenarioFolder.TO_RENAME)
		{
			string restartLog;
			Workbench.GetAbsolutePath(PROFILE_DIR + "/restart.log", restartLog, false);
			notes += string.Format("Le dossier du mod n'a pas pu être renommé en %1 (dossier utilisé par un autre programme ?). Détails : %2\n", modName, ToWindowsPath(restartLog));
		}

		//--- Sub-scene
		if (!Workbench.OpenModule(WorldEditor))
		{
			Workbench.Dialog(TITLE, "Impossible d'ouvrir le World Editor.");
			return;
		}

		string error;
		ResourceName world = CreateSubScene(fileSystem, modName, state.m_sParentWorld, error);
		if (world.IsEmpty())
		{
			Workbench.Dialog(TITLE, "Impossible de créer le monde du scénario :\n" + error);
			return;
		}

		if (!IsSameResource(GetCurrentWorld(), world))
			RemoveEmptyDefaultLayer(fileSystem, modName);

		if (!OpenWorld(world))
		{
			Workbench.Dialog(TITLE, "Le monde n'a pas pu être ouvert :\n" + world.GetPath() + "\nOuvrez-le à la main puis relancez le plugin.");
			return;
		}

		//--- Layers of the operation; Game Mode Setup creates its entities in the active layer: Composants_Mission
		WorldEditorAPI api = SCR_WorldEditorToolHelper.GetWorldEditorAPI();
		if (api)
		{
			CreateLayers(api);
			string systemsLayer = FindLayer(api, LAYER_SYSTEMS);
			if (!systemsLayer.IsEmpty())
				api.SetActiveSubsceneLayer(api.GetCurrentSubScene(), systemsLayer);
		}

		//--- CK configs of the mod
		ResourceName briefing = CreateBriefingConfig(fileSystem, modName, state);
		ResourceName loadouts = CreateLoadoutsConfig(fileSystem, modName);
		if (!AddDefaultLoadouts(loadouts))
			notes += string.Format("Les loadouts par défaut n'ont pas pu être ajoutés (%1).\n", GetLoadoutsPath(modName));

		//--- Game Mode Setup, forced on Game Master, mission header named after the operation
		if (!HasHeaderTextures(fileSystem))
			notes += "Images du chargement et de l'aperçu absentes du mod : copiez le dossier UI du plugin à côté de Scripts, puis relancez le plugin.\n";

		if (!RunGameModeSetup(state, briefing))
			return;

		//--- Elements
		RunPlacement(state, briefing, loadouts, notes);
	}

	//------------------------------------------------------------------------------------------------
	//! Open the world in the World Editor (a world registered a moment ago cannot be opened at once: retry)
	static bool OpenWorld(ResourceName world)
	{
		if (IsSameResource(GetCurrentWorld(), world))
			return true;

		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		for (int attempt = 0; attempt < 20; attempt++)
		{
			if (worldEditor.SetOpenedResource(world) && SCR_WorldEditorToolHelper.IsWorldLoaded())
				return true;

			Sleep(250);
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Game Mode Setup only scans the scenario sub-scene: on a terrain that already has a map, an AI world or a
	//! perception manager, it creates a second one ("Multiple map entities present!"). Delete the copies of the sub-scene.
	//! \return number of deleted entities
	static int RemoveDuplicateSingletons(notnull WorldEditorAPI api)
	{
		array<typename> singletons = { MapEntity, AIWorld, PerceptionManager };
		int current = api.GetCurrentSubScene();

		//--- Singleton types already in the parent worlds
		array<typename> inParents = {};
		for (int subScene = 0; subScene < current; subScene++)
		{
			for (int i = 0, count = api.GetEntityCount(subScene); i < count; i++)
			{
				IEntitySource parentSource = api.GetEntity(subScene, i);
				if (!parentSource)
					continue;

				typename parentType = parentSource.GetClassName().ToType();
				if (!parentType)
					continue;

				foreach (typename singleton : singletons)
				{
					if (parentType.IsInherited(singleton) && !inParents.Contains(singleton))
						inParents.Insert(singleton);
				}
			}
		}

		if (inParents.IsEmpty())
			return 0;

		array<IEntitySource> toDelete = {};
		for (int j = 0, currentCount = api.GetEntityCount(current); j < currentCount; j++)
		{
			IEntitySource entitySource = api.GetEntity(current, j);
			if (!entitySource)
				continue;

			typename type = entitySource.GetClassName().ToType();
			if (!type)
				continue;

			foreach (typename parentSingleton : inParents)
			{
				if (type.IsInherited(parentSingleton))
				{
					toDelete.Insert(entitySource);
					break;
				}
			}
		}

		foreach (IEntitySource duplicate : toDelete)
		{
			Print(TITLE + ": doublon supprimé (déjà dans le terrain) : " + duplicate.GetClassName(), LogLevel.NORMAL);
			api.DeleteEntity(duplicate);
		}

		return toDelete.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! \return path of the layer of the current sub-scene with this name (also inside a folder), empty if missing
	static string FindLayer(notnull WorldEditorAPI api, string name)
	{
		array<string> layers = {};
		api.GetSubsceneFolders(api.GetCurrentSubScene(), layers);
		foreach (string layer : layers)
		{
			if (layer == name || layer.EndsWith("/" + name))
				return layer;
		}

		return string.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! \return path of the layer, created if missing (GetSubsceneLayerId returns the default layer for a missing name)
	static string EnsureLayer(notnull WorldEditorAPI api, string name)
	{
		string layer = FindLayer(api, name);
		if (!layer.IsEmpty())
			return layer;

		return api.CreateSubsceneLayer(api.GetCurrentSubScene(), name);
	}

	//------------------------------------------------------------------------------------------------
	//! Composants_Mission (game mode, managers, CK systems), Base_Avancee (FOB), Jour_1 to Jour_4 (one per Friday)
	static void GetLayerNames(notnull array<string> names)
	{
		names.Insert(LAYER_SYSTEMS);
		names.Insert(LAYER_BASE);
		for (int day = 1; day <= DAY_COUNT; day++)
		{
			names.Insert(LAYER_DAY + day.ToString());
		}
	}

	//------------------------------------------------------------------------------------------------
	static void CreateLayers(notnull WorldEditorAPI api)
	{
		array<string> names = {};
		GetLayerNames(names);
		foreach (string name : names)
		{
			EnsureLayer(api, name);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! The World Editor has no script API to delete a layer: the empty "default" layer of a world created by an older
	//! version of the plugin is removed by deleting its file, while the world is closed
	//! \return true when the file was deleted
	static bool RemoveEmptyDefaultLayer(string fileSystem, string worldName)
	{
		string worldPath = fileSystem + "Worlds/" + worldName + "/" + worldName + ".ent";
		string absWorld;
		if (fileSystem.IsEmpty() || !Workbench.GetAbsolutePath(worldPath, absWorld, true))
			return false;

		string absLayers = GetDirectory(CleanPath(absWorld)) + worldName + "_Layers/";
		string defaultLayer = absLayers + "default.layer";
		if (!FileIO.FileExists(defaultLayer) || !FileIO.FileExists(absLayers + LAYER_SYSTEMS + ".layer"))
			return false;

		//--- Entities in it: kept
		FileHandle file = FileIO.OpenFile(defaultLayer, FileMode.READ);
		if (!file)
			return false;

		int length = file.GetLength();
		file.Close();
		if (length > 0)
			return false;

		if (!FileIO.DeleteFile(defaultLayer))
			Workbench.RunCmd("cmd /c del /q \"" + ToWindowsPath(defaultLayer) + "\"", true);

		return !FileIO.FileExists(defaultLayer);
	}

	//------------------------------------------------------------------------------------------------
	//! Move the scenario entities still in the default layer: game mode and managers to Composants_Mission,
	//! elements of the plugin to their layer. Entities in another layer are left where the scenario maker put them.
	//! \return number of moved entities
	static int OrganizeLayers(notnull WorldEditorAPI api)
	{
		int subScene = api.GetCurrentSubScene();
		array<ref CK_ConfigurationScenarioItem> items = {};
		GetItems(items);

		array<typename> systemTypes = { BaseGameMode, PerceptionManager, AIWorld, MapEntity };
		array<IEntitySource> toMove = {};
		array<string> targets = {};
		for (int i = 0, count = api.GetEntityCount(subScene); i < count; i++)
		{
			IEntitySource entitySource = api.GetEntity(subScene, i);
			if (!entitySource || entitySource.GetParent() || api.GetEntitySubsceneLayer(subScene, entitySource) != "default")
				continue;

			string target;
			typename type = entitySource.GetClassName().ToType();
			if (type)
			{
				foreach (typename systemType : systemTypes)
				{
					if (type.IsInherited(systemType))
					{
						target = LAYER_SYSTEMS;
						break;
					}
				}
			}

			if (target.IsEmpty())
			{
				BaseContainer ancestor = entitySource.GetAncestor();
				while (ancestor && target.IsEmpty())
				{
					string guid = GetResourceGuid(ancestor.GetResourceName());
					foreach (CK_ConfigurationScenarioItem item : items)
					{
						if (GetResourceGuid(item.m_sPrefab) == guid)
						{
							target = item.m_sLayer;
							break;
						}
					}

					ancestor = ancestor.GetAncestor();
				}
			}

			if (target.IsEmpty())
				continue;

			toMove.Insert(entitySource);
			targets.Insert(target);
		}

		foreach (int j, IEntitySource movedSource : toMove)
		{
			api.SetEntitySubsceneLayer(subScene, movedSource, EnsureLayer(api, targets[j]));
		}

		return toMove.Count();
	}

	//------------------------------------------------------------------------------------------------
	//! <fileSystem>Worlds/<name>/<name>.ent: sub-scene of parentWorld, created and registered if missing
	//! \return resource name of the world, empty on error
	static ResourceName CreateSubScene(string fileSystem, string worldName, string parentWorld, out string error)
	{
		string worldPath = fileSystem + "Worlds/" + worldName + "/" + worldName + ".ent";
		string absWorld;
		if (fileSystem.IsEmpty() || !Workbench.GetAbsolutePath(worldPath, absWorld, false))
		{
			error = worldPath;
			return ResourceName.Empty;
		}

		absWorld = CleanPath(absWorld);
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		if (!FileIO.FileExists(absWorld))
		{
			if (GetResourceGuid(parentWorld).IsEmpty())
			{
				error = "terrain non valide : " + parentWorld;
				return ResourceName.Empty;
			}

			string absLayers = GetDirectory(absWorld) + worldName + "_Layers";
			FileIO.MakeDirectory(absLayers);

			FileHandle worldFile = FileIO.OpenFile(absWorld, FileMode.WRITE);
			if (!worldFile)
			{
				error = absWorld;
				return ResourceName.Empty;
			}

			worldFile.WriteLine("SubScene {");
			worldFile.WriteLine(" Parent \"" + parentWorld + "\"");
			worldFile.WriteLine("}");
			worldFile.Close();

			//--- The layers of the operation, without Workbench's "default" layer
			array<string> layerNames = {};
			GetLayerNames(layerNames);
			foreach (string layerName : layerNames)
			{
				FileHandle layerFile = FileIO.OpenFile(absLayers + "/" + layerName + ".layer", FileMode.WRITE);
				if (layerFile)
					layerFile.Close();
			}

			resourceManager.RegisterResourceFile(absWorld, false);
			Print(TITLE + ": sous-scène créée : " + absWorld, LogLevel.NORMAL);
		}

		return GetResourceNameFromFile(absWorld, worldPath);
	}

	//------------------------------------------------------------------------------------------------
	//! "{GUID}path" of a registered file (meta file), else Workbench's resource name of the relative path
	static ResourceName GetResourceNameFromFile(string absolutePath, string relativePath)
	{
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		MetaFile meta = resourceManager.GetMetaFile(absolutePath);
		if (meta)
			return meta.GetResourceID();

		return Workbench.GetResourceName(relativePath);
	}

	//------------------------------------------------------------------------------------------------
	//! Save a container to <fileSystem><relativePath> and register it
	//! \return its resource name, empty on error
	static ResourceName SaveNewConfig(notnull BaseContainer container, string fileSystem, string relativePath)
	{
		string path = fileSystem + relativePath;
		string absolutePath;
		if (!Workbench.GetAbsolutePath(path, absolutePath, false))
			return ResourceName.Empty;

		absolutePath = CleanPath(absolutePath);
		FileIO.MakeDirectory(GetDirectory(absolutePath));
		if (!BaseContainerTools.SaveContainer(container, ResourceName.Empty, path))
			return ResourceName.Empty;

		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		resourceManager.RegisterResourceFile(absolutePath, false);
		return GetResourceNameFromFile(absolutePath, path);
	}

	//------------------------------------------------------------------------------------------------
	//! Existing config of the mod (kept as is), empty if missing
	static ResourceName GetExistingConfig(string fileSystem, string relativePath)
	{
		string absolutePath;
		if (!Workbench.GetAbsolutePath(fileSystem + relativePath, absolutePath, true))
			return ResourceName.Empty;

		return GetResourceNameFromFile(CleanPath(absolutePath), fileSystem + relativePath);
	}

	//------------------------------------------------------------------------------------------------
	static string GetBriefingPath(string modName)
	{
		return "Configs/CK/" + modName + "_Briefing.conf";
	}

	//------------------------------------------------------------------------------------------------
	static string GetLoadoutsPath(string modName)
	{
		return "Configs/CK/" + modName + "_Loadouts.conf";
	}

	//------------------------------------------------------------------------------------------------
	//! CK_BriefingConfig (CK Core) of the operation: title, subtitle with the date, base of the order
	static ResourceName CreateBriefingConfig(string fileSystem, string modName, notnull CK_ConfigurationScenarioState state)
	{
		ResourceName existing = GetExistingConfig(fileSystem, GetBriefingPath(modName));
		if (!existing.IsEmpty())
			return existing;

		Resource resource = BaseContainerTools.CreateContainer("CK_BriefingConfig");
		if (!resource || !resource.IsValid())
			return ResourceName.Empty;

		BaseContainer container = resource.GetResource().ToBaseContainer();
		container.Set("m_sTitle", state.m_sOperation);
		container.Set("m_sSubtitle", "Commando Kieffer 2004 - Ordre d'opération du " + state.GetDateText());
		container.Set("m_sText", "# Situation\nÀ compléter.\n\n# Mission\nÀ compléter.\n\n# Exécution\nÀ compléter.\n\n# Logistique\nÀ compléter.\n\n# Transmissions\nÀ compléter.");
		return SaveNewConfig(container, fileSystem, GetBriefingPath(modName));
	}

	//------------------------------------------------------------------------------------------------
	//! CK_MissionLoadoutList (CK Core) of the operation, created empty (the default loadouts are added by AddDefaultLoadouts)
	static ResourceName CreateLoadoutsConfig(string fileSystem, string modName)
	{
		ResourceName existing = GetExistingConfig(fileSystem, GetLoadoutsPath(modName));
		if (!existing.IsEmpty())
			return existing;

		Resource resource = BaseContainerTools.CreateContainer("CK_MissionLoadoutList");
		if (!resource || !resource.IsValid())
			return ResourceName.Empty;

		return SaveNewConfig(resource.GetResource().ToBaseContainer(), fileSystem, GetLoadoutsPath(modName));
	}

	//------------------------------------------------------------------------------------------------
	//! Default loadouts of the CK menu, exported from the CK menu (Arsenal > Export). Their items come from CK Equipment,
	//! CK Weapons, GRS and RHS Status Quo: these mods are not dependencies of the scenario, the items of a mod that is
	//! not loaded are just missing from the loadout.
	static void GetDefaultLoadouts(notnull array<ref CK_ConfigurationScenarioLoadout> loadouts)
	{
		loadouts.Clear();
		loadouts.Insert(new CK_ConfigurationScenarioLoadout(DEFAULT_LOADOUT_PLACE_DARMES, "Treillis BME, Glock 17 (3 chargeurs), radio, SICS, télémètre Vector", GetPlaceDarmesCode()));
		loadouts.Insert(new CK_ConfigurationScenarioLoadout(DEFAULT_LOADOUT_RECRUE, "FAMAS (8 chargeurs), Glock 17, gilet AVS, casque Ops-Core, radio, SICS", GetRecrueCode()));
	}

	//------------------------------------------------------------------------------------------------
	//! "Place d'armes" (the code is cut in parts: shorter lines)
	static string GetPlaceDarmesCode()
	{
		string code = "CKL1:{%22version%22:1,%22name%22:%22Place d'armes%22,%22scenario%22:%22Base Entraînement%22,%22date%22:%2211/11/2026 01:16%22,%22faction%22:%22US%22,%22data%22:%22{%5C%22faction%5C%22:%5C%22US%5C%22,%5C%22arsenalLoadout%5C%22:{%5C%22prefab%5C%22:%5C%2226A9756790131354%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22EquipedWeaponStorageComponent:520EA1D2DB118C70%5C%22,%5C%22slots%5C%22:{}},{%5C%22id%5C%22:%5C%22SCR_EquipmentStorageComponent:56B49DA9722B635D%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%2203810BEF182ADA0B%5C%22},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%2278ED4FEF62BBA728%5C%22}}},{%5C%22id%5C%22:%5C%22SCR_CharacterInventoryStorageComponent:520EA1D2DB118DAC%5C%22,%5C%22slots%5C%22:{%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%223BA4B7BE6538D6E6%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22ClothNodeStorageComponent:5222CB01D5B9A306%5C%22,%5C%22slots%5C%22:{%5C%";
		code += "224%5C%22:{%5C%22prefab%5C%22:%5C%22D5982F3150A75A97%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_UniversalInventoryStorageComponent:658A2C52FF987D55%5C%22,%5C%22slots%5C%22:{}}]},%5C%225%5C%22:{%5C%22prefab%5C%22:%5C%222518FCA33DD1BEA6%5C%22}}}]},%5C%223%5C%22:{%5C%22prefab%5C%22:%5C%224221022703A2852D%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_UniversalInventoryStorageComponent:5222CB01D5B9A306%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%2285427A0E24357BE6%5C%22},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%2261D4F80E49BF9B12%5C%22}}}]},%5C%224%5C%22:{%5C%22prefab%5C%22:%5C%2298388F9E1E801A42%5C%22},%5C%229%5C%22:{%5C%22prefab%5C%22:%5C%227EDD40FF83EDA44F%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22ClothNodeStorageComponent:65CD429CD4A3549B%5C%22,%5C%22slots%5C%22:{%5C%224%5C%22:{%5C%22prefab%5C%22:%5C%2228480419FE4C4063%5C%22,%5C%22storages%5C%22:";
		code += "[{%5C%22id%5C%22:%5C%22GRS_EquipmentStorageComponent:65CD429B184CE5AA%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%22C81DBB62917BCE20%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_WeaponAttachmentsStorageComponent:51F080D5CE45A1A2%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%223C9D22848B84856E%5C%22}}}]}}}]},%5C%225%5C%22:{%5C%22prefab%5C%22:%5C%227591612DB8B27A5D%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_EquipmentStorageComponent:65E71517FCDC40DB%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%223C9D22848B84856E%5C%22}}}]},%5C%226%5C%22:{%5C%22prefab%5C%22:%5C%227591612DB8B27A5D%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_EquipmentStorageComponent:65E71517FCDC40DB%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%223C9D22848B84856E%5C%22}}}]},%5C%2211%5C%22:{%5C%22prefab%5C%22:%5C%22F0DBA15DD3C";
		code += "EE5A6%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_PouchStorageComponent:697CF62538A99DF1%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%22D69684663E89D6EA%5C%22},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%22A81F501D3EF6F38E%5C%22},%5C%222%5C%22:{%5C%22prefab%5C%22:%5C%22D70216B1B2889129%5C%22},%5C%223%5C%22:{%5C%22prefab%5C%22:%5C%22A81F501D3EF6F38E%5C%22}}}]},%5C%2212%5C%22:{%5C%22prefab%5C%22:%5C%22EFF9BF2AFF9E4856%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_UniversalInventoryStorageComponent:65CC0BEE0F3A796A%5C%22,%5C%22slots%5C%22:{}}]}}}]},%5C%2213%5C%22:{%5C%22prefab%5C%22:%5C%229A0B49CB579AFBEF%5C%22}}}],%5C%22loadoutLabels%5C%22:[122,46,70,2,11]}}%22}";
		return code;
	}

	//------------------------------------------------------------------------------------------------
	//! "Recrue"
	static string GetRecrueCode()
	{
		string code = "CKL1:{%22version%22:1,%22name%22:%22Recrue%22,%22scenario%22:%22Base Entraînement%22,%22date%22:%2211/11/2026 01:17%22,%22faction%22:%22US%22,%22data%22:%22{%5C%22faction%5C%22:%5C%22US%5C%22,%5C%22arsenalLoadout%5C%22:{%5C%22prefab%5C%22:%5C%2226A9756790131354%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22EquipedWeaponStorageComponent:520EA1D2DB118C70%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%2274F848077D67904A%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_WeaponAttachmentsStorageComponent:51F080D5CE45A1A2%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%225523186F55EF616A%5C%22},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%2208286DDBB1F33FF1%5C%22},%5C%224%5C%22:{%5C%22prefab%5C%22:%5C%22E52C9791E1554A5F%5C%22}}}]}}},{%5C%22id%5C%22:%5C%22SCR_EquipmentStorageComponent:56B49DA9722B635D%5C%22,%5C%22slots%5C%22:{%5C%221%5C%22:{%5C%22prefab%5C%22:%5C";
		code += "%2278ED4FEF62BBA728%5C%22}}},{%5C%22id%5C%22:%5C%22SCR_CharacterInventoryStorageComponent:520EA1D2DB118DAC%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%226AC0597EBA417C43%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22RHS_HelmetNodeStorageComponent:5DB13C5777E6E695%5C%22,%5C%22slots%5C%22:{%5C%225%5C%22:{%5C%22prefab%5C%22:%5C%222162B5F1B9F5F6BB%5C%22}}}]},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%223BA4B7BE6538D6E6%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22ClothNodeStorageComponent:5222CB01D5B9A306%5C%22,%5C%22slots%5C%22:{%5C%224%5C%22:{%5C%22prefab%5C%22:%5C%22D5982F3150A75A97%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_UniversalInventoryStorageComponent:658A2C52FF987D55%5C%22,%5C%22slots%5C%22:{}}]}}}]},%5C%222%5C%22:{%5C%22prefab%5C%22:%5C%22CF8699074C8FAE04%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_ArmorVestStorageComponent:697F5ADB5D0";
		code += "D37AA%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%2212630DE6380559FE%5C%22},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%2258BE0FBCD10DB165%5C%22},%5C%222%5C%22:{%5C%22prefab%5C%22:%5C%22C861D87A7F00D896%5C%22},%5C%223%5C%22:{%5C%22prefab%5C%22:%5C%227721B26F70C1BCDE%5C%22},%5C%224%5C%22:{%5C%22prefab%5C%22:%5C%221A7333876077FCF0%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_EquipmentStorageComponent:697CF622FFAE70CE%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%225523186F55EF616A%5C%22},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%225523186F55EF616A%5C%22},%5C%222%5C%22:{%5C%22prefab%5C%22:%5C%225523186F55EF616A%5C%22}}}]},%5C%229%5C%22:{%5C%22prefab%5C%22:%5C%222518FCA33DD1BEA6%5C%22},%5C%2213%5C%22:{%5C%22prefab%5C%22:%5C%22A243C5E2B58E4AA0%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_UniversalInventoryStorageComponent:697E4650FD44EE2A%5C%22,%5C%";
		code += "22slots%5C%22:{}}]},%5C%2214%5C%22:{%5C%22prefab%5C%22:%5C%22140F46DF06586070%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_EquipmentStorageComponent:6A3A0176D88DF248%5C%22,%5C%22slots%5C%22:{%5C%222%5C%22:{%5C%22prefab%5C%22:%5C%229DB69176CEF0EE97%5C%22}}}]},%5C%2219%5C%22:{%5C%22prefab%5C%22:%5C%229FF96CE108DDA22B%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_EquipmentStorageComponent:697CF639B780F502%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%225523186F55EF616A%5C%22},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%225523186F55EF616A%5C%22}}}]},%5C%2220%5C%22:{%5C%22prefab%5C%22:%5C%229FF96CE108DDA22B%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_EquipmentStorageComponent:697CF639B780F502%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%225523186F55EF616A%5C%22},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%225523186F55EF616A%5C%22}}}]},%5C%2242";
		code += "%5C%22:{%5C%22prefab%5C%22:%5C%22968A2E9871419162%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_UniversalInventoryStorageComponent:697F3B6A10BD9A49%5C%22,%5C%22slots%5C%22:{}}]}}}]},%5C%223%5C%22:{%5C%22prefab%5C%22:%5C%224221022703A2852D%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_UniversalInventoryStorageComponent:5222CB01D5B9A306%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%22D69684663E89D6EA%5C%22},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%2261D4F80E49BF9B12%5C%22},%5C%222%5C%22:{%5C%22prefab%5C%22:%5C%2285427A0E24357BE6%5C%22},%5C%223%5C%22:{%5C%22prefab%5C%22:%5C%226E35D94130954509%5C%22}}}]},%5C%224%5C%22:{%5C%22prefab%5C%22:%5C%2298388F9E1E801A42%5C%22},%5C%229%5C%22:{%5C%22prefab%5C%22:%5C%227EDD40FF83EDA44F%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22ClothNodeStorageComponent:65CD429CD4A3549B%5C%22,%5C%22slots%5C%22:{%5C%224%5C%22:{%5C%22pr";
		code += "efab%5C%22:%5C%2228480419FE4C4063%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_EquipmentStorageComponent:65CD429B184CE5AA%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%22C81DBB62917BCE20%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_WeaponAttachmentsStorageComponent:51F080D5CE45A1A2%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%223C9D22848B84856E%5C%22}}}]}}}]},%5C%225%5C%22:{%5C%22prefab%5C%22:%5C%227591612DB8B27A5D%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_EquipmentStorageComponent:65E71517FCDC40DB%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%223C9D22848B84856E%5C%22}}}]},%5C%226%5C%22:{%5C%22prefab%5C%22:%5C%227591612DB8B27A5D%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_EquipmentStorageComponent:65E71517FCDC40DB%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%223C9D22848B84856E%5C";
		code += "%22}}}]},%5C%2211%5C%22:{%5C%22prefab%5C%22:%5C%22F0DBA15DD3CEE5A6%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22GRS_PouchStorageComponent:697CF62538A99DF1%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%22A81F501D3EF6F38E%5C%22},%5C%221%5C%22:{%5C%22prefab%5C%22:%5C%22A81F501D3EF6F38E%5C%22},%5C%222%5C%22:{%5C%22prefab%5C%22:%5C%22D70216B1B2889129%5C%22}}}]},%5C%2212%5C%22:{%5C%22prefab%5C%22:%5C%22240ADA340097707B%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_EquipmentStorageComponent:697414F4E6769F13%5C%22,%5C%22slots%5C%22:{%5C%220%5C%22:{%5C%22prefab%5C%22:%5C%22C9A4E563205BAC75%5C%22,%5C%22storages%5C%22:[{%5C%22id%5C%22:%5C%22SCR_UniversalInventoryStorageComponent:65CC0BEE0F3F0545%5C%22,%5C%22slots%5C%22:{}}]}}},{%5C%22id%5C%22:%5C%22GRS_PouchStorageComponent:6970D6BE302BB3D9%5C%22,%5C%22slots%5C%22:{}}]}}}]},%5C%2213%5C%22:{%5C%22prefab%5C%22:%5C%22D38F";
		code += "6F82BF7F83B8%5C%22}}}],%5C%22loadoutLabels%5C%22:[122,46,70,2,11],%5C%22activeWeaponIdx%5C%22:0}}%22}";
		return code;
	}

	//------------------------------------------------------------------------------------------------
	//! Default loadouts first in the loadouts of the operation (CK_MissionLoadoutList), each one when no loadout has its name.
	//! The "Tenue de briefing" of the previous versions becomes the first default loadout (same place in the list; its prefab
	//! file stays in the mod). The loaded config is changed in place: Workbench keeps it in memory, a new container saved
	//! to the file would leave the old loadouts in Workbench until it restarts. No BaseContainerList.Remove: it moves the
	//! last loadout in place of the removed one.
	//! \return true when the default loadouts are in the list
	static bool AddDefaultLoadouts(ResourceName loadouts)
	{
		if (loadouts.IsEmpty())
			return false;

		Resource listResource = BaseContainerTools.LoadContainer(loadouts);
		if (!listResource || !listResource.IsValid())
			return false;

		array<ref CK_ConfigurationScenarioLoadout> defaults = {};
		GetDefaultLoadouts(defaults);

		BaseContainer list = listResource.GetResource().ToBaseContainer();
		BaseContainerList current = list.GetObjectArray("m_aLoadouts");
		BaseContainerList loadoutList = list.SetObjectArray("m_aLoadouts");
		if (!loadoutList)
			return false;

		bool changed;

		//--- Loadouts inherited from a parent config: copied into the new list
		if (loadoutList.Count() == 0 && current && current.Count() > 0)
		{
			for (int i = 0, count = current.Count(); i < count; i++)
			{
				loadoutList.Insert(current.Get(i));
			}

			changed = true;
		}

		array<string> names = {};
		int oldOutfit = -1;
		for (int j = 0, listCount = loadoutList.Count(); j < listCount; j++)
		{
			string name;
			loadoutList.Get(j).Get("m_sName", name);
			names.Insert(name);
			if (oldOutfit < 0 && IsOldOutfitLoadout(loadoutList.Get(j)))
				oldOutfit = j;
		}

		array<string> factionKeys = { FACTION_KEY };
		if (oldOutfit >= 0 && !names.Contains(defaults[0].m_sName))
		{
			BaseContainer outfit = loadoutList.Get(oldOutfit);
			SetDefaultLoadout(outfit, defaults[0], factionKeys);
			outfit.ClearVariable("m_sCharacterPrefab");
			names[oldOutfit] = defaults[0].m_sName;
			changed = true;
		}

		//--- Missing default loadouts, in their order, before the other loadouts
		array<ref Resource> addedResources = {};		//!< keeps the new containers alive until the list is saved
		int insertAt;
		foreach (CK_ConfigurationScenarioLoadout defaultLoadout : defaults)
		{
			int index = names.Find(defaultLoadout.m_sName);
			if (index >= 0)
			{
				insertAt = index + 1;
				continue;
			}

			Resource loadoutResource = BaseContainerTools.CreateContainer("CK_MissionLoadout");
			if (!loadoutResource || !loadoutResource.IsValid())
				return false;

			addedResources.Insert(loadoutResource);
			BaseContainer loadout = loadoutResource.GetResource().ToBaseContainer();
			SetDefaultLoadout(loadout, defaultLoadout, factionKeys);
			loadoutList.Insert(loadout, insertAt);
			names.InsertAt(defaultLoadout.m_sName, insertAt);
			insertAt++;
			changed = true;
		}

		if (!changed)
			return true;

		return BaseContainerTools.SaveContainer(list, loadouts);
	}

	//------------------------------------------------------------------------------------------------
	static void SetDefaultLoadout(notnull BaseContainer loadout, notnull CK_ConfigurationScenarioLoadout defaultLoadout, array<string> factionKeys)
	{
		loadout.Set("m_sName", defaultLoadout.m_sName);
		loadout.Set("m_sDescription", defaultLoadout.m_sDescription);
		loadout.Set("m_sLoadoutCode", defaultLoadout.m_sCode);
		loadout.Set("m_aFactionKeys", factionKeys);
	}

	//------------------------------------------------------------------------------------------------
	//! "Tenue de briefing" made by the previous versions of the plugin (name and prefab)
	static bool IsOldOutfitLoadout(BaseContainer loadout)
	{
		if (!loadout)
			return false;

		string name;
		ResourceName prefab;
		loadout.Get("m_sName", name);
		loadout.Get("m_sCharacterPrefab", prefab);
		return name == OLD_OUTFIT_NAME && prefab.EndsWith(OLD_OUTFIT_PREFAB_SUFFIX);
	}

	//------------------------------------------------------------------------------------------------
	//! Mission header of the loaded world, created from the Game Master template if missing:
	//! name = operation, author = scénaristes, CK description and pictures, CK Briefing = briefing (when empty).
	//! Called by CK_ConfigurationScenario_GameModeSetup.
	//! \return true when the header exists
	static bool CreateMissionHeader(ResourceName template, notnull CK_ConfigurationScenarioState state, ResourceName briefing, out string message)
	{
		string worldPath = SCR_WorldEditorToolHelper.GetWorldPath();
		if (worldPath.IsEmpty())
			return false;

		string fileSystem = SCR_AddonTool.ToFileSystem(FilePath.FileSystemNameFromFileName(worldPath));
		string worldName = FilePath.StripExtension(FilePath.StripPath(worldPath));
		string headerPath = fileSystem + "Missions/" + worldName + ".conf";
		ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
		bool hasTextures = HasHeaderTextures(fileSystem);
		string operation = state.m_sOperation;

		ResourceName header = FindMissionHeader(fileSystem, GetCurrentWorld());
		if (!header.IsEmpty())
		{
			Resource headerResource = BaseContainerTools.LoadContainer(header);
			if (!headerResource || !headerResource.IsValid())
				return false;

			BaseContainer headerContainer = headerResource.GetResource().ToBaseContainer();
			SetMissionHeaderValues(headerContainer, state, hasTextures);
			ResourceName currentBriefing;
			headerContainer.Get("m_sCK_Briefing", currentBriefing);
			if (currentBriefing.IsEmpty() && !briefing.IsEmpty())
				headerContainer.Set("m_sCK_Briefing", briefing);

			if (!BaseContainerTools.SaveContainer(headerContainer, header))
				return false;

			message = string.Format("Mission header : %1\nNom : %2\nAuteur : %3", header.GetPath(), operation, state.GetAuthors());
			return true;
		}

		//--- New header from the template (like GameModeSetupConfig.GenerateMissionHeader)
		Resource templateResource = Resource.Load(template);
		if (!templateResource || !templateResource.IsValid())
			return false;

		BaseContainer missionHeader = templateResource.GetResource().ToBaseContainer().GetObject("m_MissionHeader");
		if (!missionHeader)
			return false;

		string absWorld;
		Workbench.GetAbsolutePath(worldPath, absWorld, true);
		MetaFile worldMeta = resourceManager.GetMetaFile(absWorld);
		if (!worldMeta)
			return false;

		missionHeader.Set("World", worldMeta.GetResourceID());
		SetMissionHeaderValues(missionHeader, state, hasTextures);
		if (!briefing.IsEmpty())
			missionHeader.Set("m_sCK_Briefing", briefing);

		string absHeader;
		Workbench.GetAbsolutePath(headerPath, absHeader, false);
		absHeader = CleanPath(absHeader);
		FileIO.MakeDirectory(GetDirectory(absHeader));
		if (!BaseContainerTools.SaveContainer(missionHeader, ResourceName.Empty, headerPath))
			return false;

		resourceManager.RegisterResourceFile(absHeader, false);
		resourceManager.SetOpenedResource(headerPath);
		message = string.Format("Mission header créé : Missions/%1.conf\nNom : %2\nAuteur : %3", worldName, operation, state.GetAuthors());
		return true;
	}

	//------------------------------------------------------------------------------------------------
	//! Name = operation, author = scénaristes. Description and pictures: CK values, unless the scenario maker changed them
	//! (only unset, empty or template placeholder values are replaced).
	static void SetMissionHeaderValues(notnull BaseContainer header, notnull CK_ConfigurationScenarioState state, bool hasTextures)
	{
		header.Set("m_sName", state.m_sOperation);
		header.Set("m_sAuthor", state.GetAuthors());
		ReplaceDefault(header, "m_sDescription", HEADER_DESCRIPTION, "<Insert world description>");
		ReplaceDefault(header, "m_sIcon", HEADER_ICON, "{6CFADAEE9287D1D2}UI/Textures/WorldSelection/Default.edds");
		if (!hasTextures)
			return;

		ReplaceDefault(header, "m_sLoadingScreen", HEADER_LOADING_SCREEN, "{C58FCC06AF13075B}UI/Textures/MissionLoadingScreens/placeholder_1.edds");
		ReplaceDefault(header, "m_sPreviewImage", HEADER_PREVIEW_IMAGE, "{C58FCC06AF13075B}UI/Textures/MissionLoadingScreens/placeholder_1.edds");
	}

	//------------------------------------------------------------------------------------------------
	static void ReplaceDefault(notnull BaseContainer container, string variable, string value, string placeholder)
	{
		string current;
		container.Get(variable, current);
		if (!container.IsVariableSetDirectly(variable) || current.IsEmpty() || current == placeholder)
			container.Set(variable, value);
	}

	//------------------------------------------------------------------------------------------------
	//! Loading screen and preview textures of the plugin in the mod (UI folder of the plugin copied with Scripts)
	static bool HasHeaderTextures(string fileSystem)
	{
		return FileIO.FileExists(fileSystem + GetResourcePath(HEADER_LOADING_SCREEN)) && FileIO.FileExists(fileSystem + GetResourcePath(HEADER_PREVIEW_IMAGE));
	}

	//------------------------------------------------------------------------------------------------
	//! Mission header in <fileSystem>Missions whose World is the given world, empty if none
	static ResourceName FindMissionHeader(string fileSystem, string world)
	{
		array<ResourceName> resources = {};
		SearchResourcesFilter filter = new SearchResourcesFilter();
		filter.fileExtensions = { "conf" };
		filter.rootPath = fileSystem + "Missions";
		ResourceDatabase.SearchResources(filter, resources.Insert);

		foreach (ResourceName resourceName : resources)
		{
			Resource resource = Resource.Load(resourceName);
			if (!resource || !resource.IsValid())
				continue;

			BaseContainer container = resource.GetResource().ToBaseContainer();
			if (!container)
				continue;

			ResourceName missionWorld;
			container.Get("World", missionWorld);
			if (!missionWorld.IsEmpty() && IsSameResource(missionWorld, world))
				return resourceName;
		}

		return ResourceName.Empty;
	}

	//------------------------------------------------------------------------------------------------
	//! \return "{GUID}path" of the loaded world, empty if none
	static string GetCurrentWorld()
	{
		if (!SCR_WorldEditorToolHelper.IsWorldLoaded())
			return string.Empty;

		string worldPath = SCR_WorldEditorToolHelper.GetWorldPath();
		if (worldPath.IsEmpty())
			return string.Empty;

		string resourceName = Workbench.GetResourceName(worldPath);
		if (resourceName.IsEmpty())
			return worldPath;

		return resourceName;
	}

	//------------------------------------------------------------------------------------------------
	//! Same GUID, or same path when a GUID is missing ("{GUID}" and "$addon:" prefixes ignored)
	static bool IsSameResource(string a, string b)
	{
		if (a.IsEmpty() || b.IsEmpty())
			return false;

		string guidA = GetResourceGuid(a);
		string guidB = GetResourceGuid(b);
		if (!guidA.IsEmpty() && !guidB.IsEmpty())
			return guidA == guidB;

		return GetResourcePath(a) == GetResourcePath(b);
	}

	//------------------------------------------------------------------------------------------------
	//! "{GUID}path" -> "GUID" (upper case), empty when there is no valid GUID
	static string GetResourceGuid(string resource)
	{
		int close = resource.IndexOf("}");
		if (!resource.StartsWith("{") || close != 17)
			return string.Empty;

		string guid = resource.Substring(1, 16);
		guid.ToUpper();
		if (guid == "0000000000000000")
			return string.Empty;

		return guid;
	}

	//------------------------------------------------------------------------------------------------
	//! "{GUID}$addon:path" -> "path" (lower case)
	static string GetResourcePath(string resource)
	{
		string path = resource;
		if (path.StartsWith("{"))
		{
			int close = path.IndexOf("}");
			if (close >= 0)
				path = path.Substring(close + 1, path.Length() - close - 1);
		}

		if (path.StartsWith("$"))
		{
			int colon = path.IndexOf(":");
			if (colon >= 0)
				path = path.Substring(colon + 1, path.Length() - colon - 1);
		}

		path.ToLower();
		return path;
	}

	//------------------------------------------------------------------------------------------------
	protected bool RunGameModeSetup(notnull CK_ConfigurationScenarioState state, ResourceName briefing)
	{
		//--- Plugins cannot be created with new: use the instance Workbench created
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		CK_ConfigurationScenario_GameModeSetup gameModeSetup = CK_ConfigurationScenario_GameModeSetup.Cast(worldEditor.GetPlugin(CK_ConfigurationScenario_GameModeSetup));
		if (!gameModeSetup)
		{
			Workbench.Dialog(TITLE, "Plugin « CK - Game Mode Setup (Game Master) » introuvable.");
			return false;
		}

		while (true)
		{
			gameModeSetup.SetOperation(state, briefing);
			gameModeSetup.Run();
			gameModeSetup.SetOperation(null, ResourceName.Empty);

			//--- A terrain that already has a game mode (GM_Eden, GM_Arland...): Game Mode Setup finds it valid and creates nothing
			if (FindWorldGameMode())
				return true;

			CK_ConfigurationScenario_GameModeDialog gameModeDialog = new CK_ConfigurationScenario_GameModeDialog();
			if (!Workbench.ScriptDialog(TITLE, "Pas de mode de jeu dans le monde : Game Mode Setup n'est pas allé au bout.", gameModeDialog))
				return false;

			if (gameModeDialog.m_iResult == CK_ConfigurationScenario_GameModeDialog.RESULT_CONTINUE)
				return true;
		}

		return false;
	}

	//------------------------------------------------------------------------------------------------
	//! Scan the top-level entities of the current sub-scene
	//! \param[out] prefabGuids GUIDs of every prefab (and parent prefab) used by these entities
	//! \return true when a game mode entity is present
	static bool ScanSubScene(notnull set<string> prefabGuids)
	{
		return FindGameMode(prefabGuids) != null;
	}

	//------------------------------------------------------------------------------------------------
	//! \return game mode entity of the current sub-scene, null if none
	static IEntitySource FindGameMode(set<string> prefabGuids = null)
	{
		WorldEditorAPI api = SCR_WorldEditorToolHelper.GetWorldEditorAPI();
		if (!api)
			return null;

		IEntitySource gameMode;
		int subScene = api.GetCurrentSubScene();
		for (int i = 0, count = api.GetEntityCount(subScene); i < count; i++)
		{
			IEntitySource entitySource = api.GetEntity(subScene, i);
			if (!entitySource)
				continue;

			typename type = entitySource.GetClassName().ToType();
			if (!gameMode && type && type.IsInherited(BaseGameMode))
				gameMode = entitySource;

			if (!prefabGuids)
				continue;

			BaseContainer ancestor = entitySource.GetAncestor();
			while (ancestor)
			{
				string guid = GetResourceGuid(ancestor.GetResourceName());
				if (!guid.IsEmpty())
					prefabGuids.Insert(guid);

				ancestor = ancestor.GetAncestor();
			}
		}

		return gameMode;
	}

	//------------------------------------------------------------------------------------------------
	//! \return game mode of the world: the one of the scenario sub-scene, else the one of a parent world (e.g. GM_Eden)
	static IEntitySource FindWorldGameMode()
	{
		IEntitySource gameMode = FindGameMode();
		if (gameMode)
			return gameMode;

		WorldEditorAPI api = SCR_WorldEditorToolHelper.GetWorldEditorAPI();
		if (!api)
			return null;

		for (int subScene = api.GetCurrentSubScene() - 1; subScene >= 0; subScene--)
		{
			for (int i = 0, count = api.GetEntityCount(subScene); i < count; i++)
			{
				IEntitySource entitySource = api.GetEntity(subScene, i);
				if (!entitySource)
					continue;

				typename type = entitySource.GetClassName().ToType();
				if (type && type.IsInherited(BaseGameMode))
					return entitySource;
			}
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! \return first top-level entity of the current sub-scene made from this prefab (or a child prefab), null if none
	static IEntitySource FindEntityByPrefab(ResourceName prefab)
	{
		WorldEditorAPI api = SCR_WorldEditorToolHelper.GetWorldEditorAPI();
		if (!api)
			return null;

		string prefabGuid = GetResourceGuid(prefab);
		int subScene = api.GetCurrentSubScene();
		for (int i = 0, count = api.GetEntityCount(subScene); i < count; i++)
		{
			IEntitySource entitySource = api.GetEntity(subScene, i);
			if (!entitySource)
				continue;

			BaseContainer ancestor = entitySource.GetAncestor();
			while (ancestor)
			{
				if (GetResourceGuid(ancestor.GetResourceName()) == prefabGuid)
					return entitySource;

				ancestor = ancestor.GetAncestor();
			}
		}

		return null;
	}

	//------------------------------------------------------------------------------------------------
	//! Offsets in metres: right / forward of the point the World Editor camera looks at
	static void GetItems(notnull array<ref CK_ConfigurationScenarioItem> items)
	{
		//--- CK_Core
		items.Insert(new CK_ConfigurationScenarioItem("Système des troops (CK_Troops)", "{5C1E7A93B2D4F086}Prefabs/CK/CK_Troops.et", "CK_Troops", -2, 0, LAYER_SYSTEMS, CK_EConfigurationScenarioItem.TROOPS));
		items.Insert(new CK_ConfigurationScenarioItem("Infos de mission du menu CK (CK_MissionSettings)", "{2B7E5A93D1C40F68}Prefabs/CK/CK_MissionSettings.et", "CK_MissionSettings", 2, 0, LAYER_SYSTEMS, CK_EConfigurationScenarioItem.MISSION_SETTINGS));
		items.Insert(new CK_ConfigurationScenarioItem("Intro de la mission (CK_MissionIntro)", MISSION_INTRO_PREFAB, "CK_MissionIntro", 0, 0, LAYER_SYSTEMS));

		//--- CK_Props / CK_Structures
		items.Insert(new CK_ConfigurationScenarioItem("Estrade avec insigne CK", "{037D8AF0FEA3F550}Prefabs/Props/Military/Camps/Platform_01_CK.et", string.Empty, 0, 12, LAYER_BASE));
		items.Insert(new CK_ConfigurationScenarioItem("Dépôt de véhicules - héliport", "{1B9E7DBB225CB67A}Prefabs/Structures/Military/Garages/CK_Vehicle_Depot/CK_Vehicle_Depot_Helipad.et", string.Empty, 20, 8, LAYER_BASE));
		items.Insert(new CK_ConfigurationScenarioItem("Dépôt de véhicules - plateforme", "{B52781AA71008D77}Prefabs/Structures/Military/Garages/CK_Vehicle_Depot/CK_Vehicle_Depot_Platform.et", string.Empty, -15, 8, LAYER_BASE));

		//--- Arma Reforger
		items.Insert(new CK_ConfigurationScenarioItem("Arsenal armes", "{377EB906F591E4BA}Prefabs/Props/Military/Arsenal/AmmoBoxes/US/AmmoBoxArsenal_Weapons_US.et", string.Empty, -3, 5, LAYER_BASE));
		items.Insert(new CK_ConfigurationScenarioItem("Arsenal munitions", "{5FFD91CF507BFFFB}Prefabs/Props/Military/Arsenal/AmmoBoxes/US/AmmoBoxArsenal_Ammunition_US.et", string.Empty, -1, 5, LAYER_BASE));
		items.Insert(new CK_ConfigurationScenarioItem("Arsenal médical", "{891EF8A5FB7162F1}Prefabs/Props/Military/Arsenal/AmmoBoxes/US/AmmoBoxArsenal_Medical_US.et", string.Empty, 1, 5, LAYER_BASE));
		items.Insert(new CK_ConfigurationScenarioItem("Arsenal équipement", "{0FC1D6E9B592F75D}Prefabs/Props/Military/Arsenal/AmmoBoxes/US/AmmoBoxArsenal_Equipment_US.et", string.Empty, 3, 5, LAYER_BASE));
		items.Insert(new CK_ConfigurationScenarioItem("Point de spawn US « " + SPAWN_POINT_NAME + " »", "{5A2220DD0C16958B}Prefabs/MP/Spawning/SpawnPoint_US.et", "SpawnPoint_BaseAvancee", 0, -4, LAYER_BASE, CK_EConfigurationScenarioItem.SPAWN_POINT));
	}

	//------------------------------------------------------------------------------------------------
	protected void RunPlacement(notnull CK_ConfigurationScenarioState state, ResourceName briefing, ResourceName loadouts, string notes)
	{
		WorldEditor worldEditor = Workbench.GetModule(WorldEditor);
		WorldEditorAPI api = SCR_WorldEditorToolHelper.GetWorldEditorAPI();
		if (!worldEditor || !api || !api.GetWorld())
		{
			Workbench.Dialog(TITLE, "Aucun monde chargé dans le World Editor.");
			return;
		}

		string listPlaced;
		string listPresent;
		string listMissing;
		string listFailed;
		PlaceAndConfigure(api, briefing, loadouts, state.m_sOperation, state.GetLocation(), listPlaced, listPresent, listMissing, listFailed);

		//--- The factions can only be made playable in a game mode of the scenario itself: a sub-scene cannot save changes
		//--- to an entity of its parent world (game mode of GM_Eden, GM_Arland...)
		IEntitySource gameMode = FindWorldGameMode();
		array<string> notPlayable = SetFactionsPlayable(api);
		string factionsText = JoinFactions(GetPlayableFactions());
		if (!gameMode)
			notes += "Aucun mode de jeu dans le monde : ajoutez-en un, puis rendez les factions " + factionsText + " jouables.\n";
		else if (!FindGameMode())
			notes += string.Format("Le mode de jeu vient du terrain (%1) : le scénario ne peut pas le modifier, les factions %2 ne sont pas jouables au départ. Le Game Master les rend jouables en jeu.\n", gameMode.GetName(), factionsText);
		else if (notPlayable.Count() == 1)
			notes += string.Format("Faction %1 non jouable dans le mode de jeu %2 : cochez « Is Playable » dans son gestionnaire de factions.\n", notPlayable[0], gameMode.GetName());
		else if (notPlayable.Count() > 1)
			notes += string.Format("Factions %1 non jouables dans le mode de jeu %2 : cochez « Is Playable » dans son gestionnaire de factions.\n", JoinFactions(notPlayable), gameMode.GetName());

		bool saved = worldEditor.Save();
		state.m_bPending = false;
		state.Save();

		//--- Report: what was placed, and where the scenario maker edits the mission
		string modName = state.GetModName();
		string result = string.Format("%1 - %2\n\n", state.m_sOperation, modName);
		if (!listPlaced.IsEmpty())
			result += "Placés (sélectionnés, à déplacer ensemble si besoin) :\n" + listPlaced + "\n";

		if (!listPresent.IsEmpty())
			result += "Déjà dans le monde :\n" + listPresent + "\n";

		if (!listMissing.IsEmpty())
			result += "Introuvables (mod manquant ou pas à jour) :\n" + listMissing + "\n";

		if (!listFailed.IsEmpty())
			result += "Échec de création :\n" + listFailed + "\n";

		result += string.Format("Calques : %1 (systèmes), %2 (FOB : arsenaux, dépôts, spawn), %3 à %4 (un par vendredi). Calque actif : %3.\n\n", LAYER_SYSTEMS, LAYER_BASE, LAYER_DAY + "1", LAYER_DAY + DAY_COUNT.ToString());
		result += "À modifier dans le mod :\n";
		result += string.Format(" - monde : Worlds/%1/%1.ent\n", modName);
		result += string.Format(" - mission header (nom, auteurs, description, images, joueurs) : Missions/%1.conf\n", modName);
		if (!briefing.IsEmpty())
			result += " - briefing du menu CK : " + briefing.GetPath() + "\n";

		if (!loadouts.IsEmpty())
			result += string.Format(" - loadouts du menu CK : %1 (« %2 » et « %3 » en premier)\n", loadouts.GetPath(), DEFAULT_LOADOUT_PLACE_DARMES, DEFAULT_LOADOUT_RECRUE);

		result += " - troops : entité CK_Troops du monde (CK Core n'a pas de .conf pour les troops)\n";
		result += " - intro (titre, lieu, image) : entité CK_MissionIntro du monde\n";
		result += string.Format(" - point de spawn : entité SpawnPoint_BaseAvancee (faction %1)\n", FACTION_KEY);
		if (FindGameMode() && notPlayable.IsEmpty())
			result += string.Format(" - factions jouables : %1 (spawn et loadouts par défaut pour %2 uniquement)\n", factionsText, FACTION_KEY);

		if (!saved)
			result += "\nLe monde n'a pas pu être enregistré : enregistrez-le (Ctrl+S).\n";

		if (!notes.IsEmpty())
			result += "\n" + notes;

		CK_ConfigurationScenario_DoneDialog doneDialog = new CK_ConfigurationScenario_DoneDialog();
		if (!Workbench.ScriptDialog(TITLE, result, doneDialog))
			return;

		if (doneDialog.m_iResult == CK_ConfigurationScenario_DoneDialog.RESULT_OPEN_BRIEFING && !briefing.IsEmpty())
		{
			ResourceManager resourceManager = Workbench.GetModule(ResourceManager);
			Workbench.OpenModule(ResourceManager);
			resourceManager.SetOpenedResource(briefing);
		}
	}

	//------------------------------------------------------------------------------------------------
	//! Place the missing elements, configure CK_MissionSettings, the spawn point and the faction (one undo step)
	static void PlaceAndConfigure(notnull WorldEditorAPI api, ResourceName briefing, ResourceName loadouts, string operation, string location, out string listPlaced, out string listPresent, out string listMissing, out string listFailed)
	{
		array<ref CK_ConfigurationScenarioItem> items = {};
		GetItems(items);

		set<string> prefabGuids = new set<string>();
		ScanSubScene(prefabGuids);

		array<CK_ConfigurationScenarioItem> toPlace = {};
		foreach (CK_ConfigurationScenarioItem item : items)
		{
			if (prefabGuids.Contains(GetResourceGuid(item.m_sPrefab)))
			{
				listPresent += " - " + item.m_sLabel + "\n";
				continue;
			}

			Resource resource = Resource.Load(item.m_sPrefab);
			if (!resource || !resource.IsValid())
			{
				listMissing += " - " + item.m_sLabel + "\n";
				continue;
			}

			toPlace.Insert(item);
		}

		bool manageEditAction = !api.IsDoingEditAction();
		if (manageEditAction)
			api.BeginEntityAction(TITLE);

		RemoveDuplicateSingletons(api);
		CreateLayers(api);
		OrganizeLayers(api);

		array<IEntitySource> created = {};
		PlaceItems(api, toPlace, listPlaced, listFailed, created);

		//--- Spawn point name (only on the one created now)
		foreach (int i, IEntitySource entitySource : created)
		{
			if (entitySource && toPlace[i].m_eKind == CK_EConfigurationScenarioItem.SPAWN_POINT)
				api.SetVariableValue(entitySource, { ContainerIdPathEntry("m_Info") }, "Name", SPAWN_POINT_NAME);
		}

		//--- Briefing and loadouts of CK_MissionSettings, when not set yet
		IEntitySource missionSettings = FindEntityByPrefab("{2B7E5A93D1C40F68}Prefabs/CK/CK_MissionSettings.et");
		if (missionSettings)
		{
			SetComponentResourceIfEmpty(api, missionSettings, "CK_MissionBriefingComponent", "m_sBriefingConfig", briefing);
			SetComponentResourceIfEmpty(api, missionSettings, "CK_MissionLoadoutsComponent", "m_sLoadoutList", loadouts);
		}

		//--- Intro: the operation instead of "Commando Kieffer 2004", where it takes place
		IEntitySource intro = FindEntityByPrefab(MISSION_INTRO_PREFAB);
		if (intro && !operation.IsEmpty())
		{
			api.SetVariableValue(intro, { ContainerIdPathEntry("CK_MissionIntroComponent") }, "m_sTitle", operation);
			api.SetVariableValue(intro, { ContainerIdPathEntry("CK_MissionIntroComponent") }, "m_sLocation", location);
		}

		//--- Players can spawn without the Game Master making the factions playable
		SetFactionsPlayable(api);

		if (manageEditAction)
			api.EndEntityAction(TITLE);

		api.UpdateSelectionGui();

		//--- What the scenario maker places next belongs to a day
		string firstDay = FindLayer(api, LAYER_DAY + "1");
		if (!firstDay.IsEmpty())
			api.SetActiveSubsceneLayer(api.GetCurrentSubScene(), firstDay);
	}

	//------------------------------------------------------------------------------------------------
	static void SetComponentResourceIfEmpty(notnull WorldEditorAPI api, notnull IEntitySource entitySource, string componentClass, string variable, ResourceName value)
	{
		if (value.IsEmpty())
			return;

		IEntityComponentSource component = SCR_BaseContainerTools.FindComponentSource(entitySource, componentClass);
		if (!component)
			return;

		ResourceName current;
		component.Get(variable, current);
		if (!current.IsEmpty())
			return;

		api.SetVariableValue(entitySource, { ContainerIdPathEntry(componentClass) }, variable, value);
	}

	//------------------------------------------------------------------------------------------------
	//! Factions playable at mission start, the same as the vanilla FactionManager_USxUSSR
	static array<string> GetPlayableFactions()
	{
		array<string> factions = { "US", "USSR" };
		return factions;
	}

	//------------------------------------------------------------------------------------------------
	//! "US et USSR"
	static string JoinFactions(notnull array<string> factions)
	{
		string result;
		for (int i = 0, count = factions.Count(); i < count; i++)
		{
			if (i > 0 && i == count - 1)
				result += " et ";
			else if (i > 0)
				result += ", ";

			result += factions[i];
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	//! The Game Master faction manager (child of GameMode_Editor_Full) has every faction non-playable:
	//! make US and USSR playable in it, as FactionManager_USxUSSR does. That prefab is not added: a second
	//! faction manager would conflict with the Game Master one, which also keeps FIA and CIV for the Game Master.
	//! \return factions not made playable (all of them when the scenario has no game mode of its own)
	static array<string> SetFactionsPlayable(notnull WorldEditorAPI api)
	{
		array<string> missing = GetPlayableFactions();
		IEntitySource gameMode = FindGameMode();
		if (!gameMode)
			return missing;

		for (int i = 0, count = gameMode.GetNumChildren(); i < count; i++)
		{
			IEntitySource child = gameMode.GetChild(i);
			typename type = child.GetClassName().ToType();
			if (!type || !type.IsInherited(SCR_FactionManager))
				continue;

			BaseContainerList factions = child.GetObjectArray("Factions");
			for (int j = 0; factions && j < factions.Count(); j++)
			{
				BaseContainer faction = factions.Get(j);
				string key;
				faction.Get("FactionKey", key);
				int index = missing.Find(key);
				if (index < 0)
					continue;

				bool playable;
				faction.Get("m_bIsPlayable", playable);
				if (!playable)
				{
					playable = api.SetVariableValue(child, { ContainerIdPathEntry("Factions", j) }, "m_bIsPlayable", "1");
					factions = child.GetObjectArray("Factions");
				}

				if (playable)
					missing.RemoveOrdered(index);
			}
		}

		return missing;
	}

	//------------------------------------------------------------------------------------------------
	//! Create the entities in front of the World Editor camera, select them and show them
	//! \param[out] created entity of each item of toPlace (null on failure)
	static void PlaceItems(notnull WorldEditorAPI api, notnull array<CK_ConfigurationScenarioItem> toPlace, out string listPlaced, out string listFailed, array<IEntitySource> created = null)
	{
		BaseWorld world = api.GetWorld();
		if (!world)
			return;

		//--- Reference point: where the camera looks, entities face the camera
		vector origin, forward;
		GetCameraTarget(api, origin, forward);
		vector right = Vector(forward[2], 0, -forward[0]);

		float yaw = Math.Atan2(-forward[0], -forward[2]) * Math.RAD2DEG;
		string angles = string.Format("0 %1 0", yaw);

		bool manageEditAction = !api.IsDoingEditAction();
		if (manageEditAction)
			api.BeginEntityAction(TITLE);

		api.ClearEntitySelection();
		int subScene = api.GetCurrentSubScene();

		foreach (CK_ConfigurationScenarioItem placeItem : toPlace)
		{
			int layerId = api.GetCurrentEntityLayerId();
			if (!placeItem.m_sLayer.IsEmpty())
				layerId = api.GetSubsceneLayerId(subScene, EnsureLayer(api, placeItem.m_sLayer));

			vector position = origin + right * placeItem.m_fRight + forward * placeItem.m_fForward;
			position[1] = world.GetSurfaceY(position[0], position[2]);

			string entityName = placeItem.m_sName;
			if (!entityName.IsEmpty() && api.FindEntityByName(entityName))
				entityName = string.Empty;

			IEntitySource entitySource = api.CreateEntity(placeItem.m_sPrefab, entityName, layerId, null, position, vector.Zero);
			if (created)
				created.Insert(entitySource);

			if (!entitySource)
			{
				listFailed += " - " + placeItem.m_sLabel + "\n";
				continue;
			}

			api.SetVariableValue(entitySource, null, "angles", angles);
			api.AddToEntitySelection(entitySource);
			listPlaced += " - " + placeItem.m_sLabel + "\n";
		}

		if (manageEditAction)
			api.EndEntityAction(TITLE);

		api.UpdateSelectionGui();

		//--- Show everything that was placed
		vector cameraPosition = origin - forward * 30 + vector.Up * 22;
		vector cameraDirection = origin + forward * 8 - cameraPosition;
		cameraDirection.Normalize();
		api.SetCamera(cameraPosition, cameraDirection);
	}

	//------------------------------------------------------------------------------------------------
	//! \param[out] origin terrain point at the centre of the World Editor view (or 30 m in front of the camera)
	//! \param[out] forward horizontal camera direction (normalised)
	static void GetCameraTarget(notnull WorldEditorAPI api, out vector origin, out vector forward)
	{
		BaseWorld world = api.GetWorld();
		vector traceStart, traceEnd, traceDir;
		bool hit = api.TraceWorldPos(api.GetScreenWidth() / 2, api.GetScreenHeight() / 2, TraceFlags.WORLD, traceStart, traceEnd, traceDir);

		forward = traceDir;
		forward[1] = 0;
		if (forward.Length() < 0.01)
			forward = vector.Forward;
		else
			forward.Normalize();

		if (hit)
			origin = traceEnd;
		else
			origin = traceStart + forward * 30;

		origin[1] = world.GetSurfaceY(origin[0], origin[2]);
	}
}

//------------------------------------------------------------------------------------------------
//! Vanilla Game Mode Setup without the template page: always Game Master. Started by CK - Configuration Scénario,
//! the mission header is created (or updated) with the operation name and the CK briefing instead of the template's
//! placeholders. Registered as a plugin because Workbench plugins cannot be created with new.
[WorkbenchPluginAttribute(name: "CK - Game Mode Setup (Game Master)", description: "Game Mode Setup vanilla, toujours en Game Master", wbModules: { "WorldEditor" }, awesomeFontCode: 0xF6E8)]
class CK_ConfigurationScenario_GameModeSetup : SCR_GameModeSetupPlugin
{
	protected ref CK_ConfigurationScenarioState m_CKState;
	protected ResourceName m_sCKBriefing;

	//------------------------------------------------------------------------------------------------
	//! state null: vanilla behaviour
	void SetOperation(CK_ConfigurationScenarioState state, ResourceName briefing)
	{
		m_CKState = state;
		m_sCKBriefing = briefing;
	}

	//------------------------------------------------------------------------------------------------
	override void Run()
	{
		m_sTemplate = CK_ConfigurationScenarioPlugin.GAME_MASTER_TEMPLATE;

		string error;
		if (!LoadConfig(error))
		{
			Workbench.Dialog(CK_ConfigurationScenarioPlugin.TITLE, error);
			return;
		}

		ShowPage(SCR_EGameModeSetupPage.VALIDATION);
	}

	//------------------------------------------------------------------------------------------------
	override protected void RunMissionHeader()
	{
		//--- Started on its own: vanilla header step
		if (!m_CKState || m_CKState.m_sOperation.IsEmpty())
		{
			super.RunMissionHeader();
			return;
		}

		m_sDialogMessageMissionHeader = string.Empty;
		m_bIsMissionHeaderValid = CK_ConfigurationScenarioPlugin.CreateMissionHeader(m_sTemplate, m_CKState, m_sCKBriefing, m_sDialogMessageMissionHeader);
		if (!m_bIsMissionHeaderValid)
			m_sDialogMessageMissionHeader = "Le mission header n'a pas pu être créé.";

		//--- No page of its own (as in vanilla when the header exists): go straight to the results
		m_aPageHistory.Resize(m_aPageHistory.Count() - 1);
		ShowPage(SCR_EGameModeSetupPage.MISSION_HEADER_RESULTS);
	}
}

//------------------------------------------------------------------------------------------------
//! Configuration in progress, kept across the restart: $profile:CK_ConfigurationScenario/<project GUID>.txt
class CK_ConfigurationScenarioState
{
	string m_sProjectGuid;
	string m_sParentWorld;
	string m_sOperation;
	int m_iYear;
	int m_iMonth;
	int m_iDay;
	string m_sLocation;		//!< as typed, empty: default
	string m_sAuthors;		//!< "Jean, Paul", empty: default
	string m_sListedPath;	//!< .gproj path put in the launcher project lists before the restart
	bool m_bPending;

	static const string DEFAULT_LOCATION = "Quelque part dans le monde";
	static const string DEFAULT_AUTHORS = "Commando Kieffer";

	//------------------------------------------------------------------------------------------------
	string GetLocation()
	{
		if (m_sLocation.IsEmpty())
			return DEFAULT_LOCATION;

		return m_sLocation;
	}

	//------------------------------------------------------------------------------------------------
	string GetAuthors()
	{
		if (m_sAuthors.IsEmpty())
			return DEFAULT_AUTHORS;

		return m_sAuthors;
	}

	//------------------------------------------------------------------------------------------------
	static string GetPath(string projectGuid)
	{
		return CK_ConfigurationScenarioPlugin.PROFILE_DIR + "/" + projectGuid + ".txt";
	}

	//------------------------------------------------------------------------------------------------
	static CK_ConfigurationScenarioState Load(string projectGuid)
	{
		FileHandle file = FileIO.OpenFile(GetPath(projectGuid), FileMode.READ);
		if (!file)
			return null;

		CK_ConfigurationScenarioState state = new CK_ConfigurationScenarioState();
		state.m_sProjectGuid = projectGuid;

		string line;
		while (file.ReadLine(line) >= 0)
		{
			int separator = line.IndexOf("=");
			if (separator < 1)
				continue;

			string key = line.Substring(0, separator);
			string value = line.Substring(separator + 1, line.Length() - separator - 1);
			switch (key)
			{
				case "world": state.m_sParentWorld = value; break;
				case "operation": state.m_sOperation = value; break;
				case "year": state.m_iYear = value.ToInt(); break;
				case "month": state.m_iMonth = value.ToInt(); break;
				case "day": state.m_iDay = value.ToInt(); break;
				case "location": state.m_sLocation = value; break;
				case "authors": state.m_sAuthors = value; break;
				case "listed": state.m_sListedPath = value; break;
				case "pending": state.m_bPending = value == "1"; break;
			}
		}

		file.Close();
		return state;
	}

	//------------------------------------------------------------------------------------------------
	bool Save()
	{
		FileIO.MakeDirectory(CK_ConfigurationScenarioPlugin.PROFILE_DIR);
		FileHandle file = FileIO.OpenFile(GetPath(m_sProjectGuid), FileMode.WRITE);
		if (!file)
			return false;

		file.WriteLine("world=" + m_sParentWorld);
		file.WriteLine("operation=" + m_sOperation);
		file.WriteLine("year=" + m_iYear.ToString());
		file.WriteLine("month=" + m_iMonth.ToString());
		file.WriteLine("day=" + m_iDay.ToString());
		file.WriteLine("location=" + m_sLocation);
		file.WriteLine("authors=" + m_sAuthors);
		file.WriteLine("listed=" + m_sListedPath);
		if (m_bPending)
			file.WriteLine("pending=1");
		else
			file.WriteLine("pending=0");

		file.Close();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool HasDate()
	{
		return CK_ConfigurationScenarioPlugin.IsValidDate(m_iYear, m_iMonth, m_iDay);
	}

	//------------------------------------------------------------------------------------------------
	//! CK_Op_<year>_<month>_<day>, month and day on 2 digits: CK_Op_2026_12_02
	string GetModName()
	{
		return string.Format("CK_Op_%1_%2_%3", m_iYear, TwoDigits(m_iMonth), TwoDigits(m_iDay));
	}

	//------------------------------------------------------------------------------------------------
	static string TwoDigits(int value)
	{
		if (value >= 0 && value < 10)
			return "0" + value.ToString();

		return value.ToString();
	}

	//------------------------------------------------------------------------------------------------
	//! "2 novembre 2026"
	string GetDateText()
	{
		return string.Format("%1 %2 %3", m_iDay, CK_ConfigurationScenarioPlugin.GetMonthName(m_iMonth), m_iYear);
	}
}

//------------------------------------------------------------------------------------------------
enum CK_EConfigurationScenarioFolder
{
	RENAMED,
	TO_RENAME,
	TAKEN,
}

//------------------------------------------------------------------------------------------------
enum CK_EConfigurationScenarioItem
{
	OTHER,
	TROOPS,
	MISSION_SETTINGS,
	SPAWN_POINT,
}

//------------------------------------------------------------------------------------------------
class CK_ConfigurationScenarioItem
{
	string m_sLabel;
	ResourceName m_sPrefab;
	string m_sName;
	float m_fRight;
	float m_fForward;
	string m_sLayer;
	CK_EConfigurationScenarioItem m_eKind;

	//------------------------------------------------------------------------------------------------
	void CK_ConfigurationScenarioItem(string label, ResourceName prefab, string entityName, float right, float forward, string layer, CK_EConfigurationScenarioItem kind = CK_EConfigurationScenarioItem.OTHER)
	{
		m_sLabel = label;
		m_sPrefab = prefab;
		m_sName = entityName;
		m_fRight = right;
		m_fForward = forward;
		m_sLayer = layer;
		m_eKind = kind;
	}
}

//------------------------------------------------------------------------------------------------
//! Text edition of a .gproj: the root "Dependencies { }" block and the root ID / TITLE lines are rewritten,
//! every other line is kept as is
class CK_ConfigurationScenarioProjectFile
{
	protected string m_sPath;
	protected string m_sGuid;
	protected ref array<string> m_aLines = {};
	protected ref array<string> m_aDependencies = {};
	protected ref map<string, int> m_mValueLines = new map<string, int>();
	protected int m_iDependenciesStart = -1;
	protected int m_iDependenciesEnd = -1;
	protected int m_iInsertAt = 1;
	protected bool m_bModified;

	//------------------------------------------------------------------------------------------------
	bool Load(string path)
	{
		m_sPath = path;
		m_sGuid = string.Empty;
		m_aLines.Clear();
		m_aDependencies.Clear();
		m_mValueLines.Clear();
		m_iDependenciesStart = -1;
		m_iDependenciesEnd = -1;
		m_iInsertAt = 1;
		m_bModified = false;

		FileHandle file = FileIO.OpenFile(path, FileMode.READ);
		if (!file)
			return false;

		string line;
		while (file.ReadLine(line) >= 0)
		{
			//--- Drop a trailing carriage return (CRLF files)
			int length = line.Length();
			if (length > 0 && line.ToAscii(length - 1) == 13)
				line = line.Substring(0, length - 1);

			m_aLines.Insert(line);
		}

		file.Close();

		if (m_aLines.IsEmpty())
			return false;

		array<string> valueKeys = { "ID", "GUID", "TITLE" };
		int depth;
		bool dependenciesOpened;
		for (int i = 0, count = m_aLines.Count(); i < count; i++)
		{
			string text = m_aLines[i].Trim();
			if (depth == 1)
			{
				if (m_iDependenciesStart < 0 && StartsWithWord(text, "Dependencies"))
					m_iDependenciesStart = i;

				foreach (string key : valueKeys)
				{
					if (!StartsWithWord(text, key))
						continue;

					if (!m_mValueLines.Contains(key))
						m_mValueLines.Insert(key, i);

					m_iInsertAt = i + 1;
				}
			}

			depth += CountBraces(text);

			if (m_iDependenciesStart >= 0 && m_iDependenciesEnd < 0)
			{
				if (text.Contains("{"))
					dependenciesOpened = true;

				array<string> quoted = {};
				GetAllQuoted(text, quoted);
				foreach (string quotedGuid : quoted)
				{
					quotedGuid.ToUpper();
					if (!m_aDependencies.Contains(quotedGuid))
						m_aDependencies.Insert(quotedGuid);
				}

				if (dependenciesOpened && depth <= 1)
					m_iDependenciesEnd = i;
			}
		}

		//--- Unclosed block: do not touch the file
		if (m_iDependenciesStart >= 0 && m_iDependenciesEnd < 0)
			return false;

		m_sGuid = GetValue("GUID");
		m_sGuid.ToUpper();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	string GetPath()
	{
		return m_sPath;
	}

	//------------------------------------------------------------------------------------------------
	string GetGuid()
	{
		return m_sGuid;
	}

	//------------------------------------------------------------------------------------------------
	bool IsModified()
	{
		return m_bModified;
	}

	//------------------------------------------------------------------------------------------------
	//! Value of a root line (ID, GUID, TITLE), empty if missing
	string GetValue(string key)
	{
		int index;
		if (!m_mValueLines.Find(key, index))
			return string.Empty;

		return GetFirstQuoted(m_aLines[index].Trim());
	}

	//------------------------------------------------------------------------------------------------
	//! Set a root line (ID or TITLE), added after the header lines if missing
	void SetValue(string key, string value)
	{
		if (GetValue(key) == value)
			return;

		int index;
		if (m_mValueLines.Find(key, index))
		{
			m_aLines[index] = GetIndent(m_aLines[index]) + key + " \"" + value + "\"";
		}
		else
		{
			index = m_iInsertAt;
			m_aLines.InsertAt(" " + key + " \"" + value + "\"", index);
			foreach (string otherKey, int otherIndex : m_mValueLines)
			{
				if (otherIndex >= index)
					m_mValueLines.Set(otherKey, otherIndex + 1);
			}

			if (m_iDependenciesStart >= index)
			{
				m_iDependenciesStart++;
				m_iDependenciesEnd++;
			}

			m_mValueLines.Insert(key, index);
			m_iInsertAt = index + 1;
		}

		m_bModified = true;
	}

	//------------------------------------------------------------------------------------------------
	void GetDependencies(notnull array<string> guids)
	{
		guids.Copy(m_aDependencies);
	}

	//------------------------------------------------------------------------------------------------
	bool HasDependency(string guid)
	{
		return m_aDependencies.Contains(guid);
	}

	//------------------------------------------------------------------------------------------------
	//! \return number of dependencies added
	int AddDependencies(notnull array<string> guids)
	{
		int added;
		foreach (string guid : guids)
		{
			if (guid == m_sGuid || m_aDependencies.Contains(guid))
				continue;

			m_aDependencies.Insert(guid);
			added++;
		}

		if (added > 0)
			m_bModified = true;

		return added;
	}

	//------------------------------------------------------------------------------------------------
	//! Copy of the file as it was loaded: <directory>/<GUID>_addon.gproj.bak
	bool Backup(string directory)
	{
		FileIO.MakeDirectory(directory);
		string name = m_sGuid;
		if (name.IsEmpty())
			name = "project";

		FileHandle file = FileIO.OpenFile(directory + "/" + name + "_addon.gproj.bak", FileMode.WRITE);
		if (!file)
			return false;

		foreach (string line : m_aLines)
		{
			file.WriteLine(line);
		}

		file.Close();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	bool Save()
	{
		array<string> lines = {};
		BuildLines(lines);

		FileHandle file = FileIO.OpenFile(m_sPath, FileMode.WRITE);
		if (!file)
			return false;

		foreach (string line : lines)
		{
			file.WriteLine(line);
		}

		file.Close();
		return true;
	}

	//------------------------------------------------------------------------------------------------
	void BuildLines(notnull array<string> lines)
	{
		string indent = " ";
		int from = m_iInsertAt;
		int to = m_iInsertAt - 1;
		if (m_iDependenciesStart >= 0)
		{
			indent = GetIndent(m_aLines[m_iDependenciesStart]);
			from = m_iDependenciesStart;
			to = m_iDependenciesEnd;
		}

		for (int i = 0; i < from; i++)
		{
			lines.Insert(m_aLines[i]);
		}

		lines.Insert(indent + "Dependencies {");
		foreach (string guid : m_aDependencies)
		{
			lines.Insert(indent + " \"" + guid + "\"");
		}

		lines.Insert(indent + "}");

		for (int j = to + 1, count = m_aLines.Count(); j < count; j++)
		{
			lines.Insert(m_aLines[j]);
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static bool StartsWithWord(string text, string word)
	{
		if (!text.StartsWith(word))
			return false;

		int length = word.Length();
		if (text.Length() == length)
			return true;

		string next = text.Get(length);
		return next == " " || next == "{" || next == "\"" || text.ToAscii(length) == 9;
	}

	//------------------------------------------------------------------------------------------------
	//! "{" minus "}" outside of quotes
	protected static int CountBraces(string text)
	{
		int result;
		bool inQuotes;
		for (int i = 0, length = text.Length(); i < length; i++)
		{
			string c = text.Get(i);
			if (c == "\"")
				inQuotes = !inQuotes;
			else if (!inQuotes && c == "{")
				result++;
			else if (!inQuotes && c == "}")
				result--;
		}

		return result;
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetFirstQuoted(string text)
	{
		array<string> quoted = {};
		GetAllQuoted(text, quoted);
		if (quoted.IsEmpty())
			return string.Empty;

		return quoted[0];
	}

	//------------------------------------------------------------------------------------------------
	protected static void GetAllQuoted(string text, notnull array<string> quoted)
	{
		int start = text.IndexOf("\"");
		while (start >= 0)
		{
			int end = text.IndexOfFrom(start + 1, "\"");
			if (end < 0)
				return;

			quoted.Insert(text.Substring(start + 1, end - start - 1));
			start = text.IndexOfFrom(end + 1, "\"");
		}
	}

	//------------------------------------------------------------------------------------------------
	protected static string GetIndent(string line)
	{
		int i;
		int length = line.Length();
		while (i < length && (line.Get(i) == " " || line.ToAscii(i) == 9))
		{
			i++;
		}

		return line.Substring(0, i);
	}
}

//------------------------------------------------------------------------------------------------
// Dialogs
//------------------------------------------------------------------------------------------------

//------------------------------------------------------------------------------------------------
class CK_ConfigurationScenario_DependenciesDialog
{
	static const int RESULT_ADD = 1;
	static const int RESULT_CONTINUE = 2;

	int m_iResult;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Annuler")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Oui")]
	protected bool ButtonAdd()
	{
		m_iResult = RESULT_ADD;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Non, continuer", true)]
	protected bool ButtonContinue()
	{
		m_iResult = RESULT_CONTINUE;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
class CK_ConfigurationScenario_GuidDialog
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBoxMultiline, desc: "GUID des mods (16 caractères), un par ligne ou séparés par des virgules")]
	string m_sGUID;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Annuler")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Ajouter", true)]
	protected bool ButtonOK()
	{
		return true;
	}
}

//------------------------------------------------------------------------------------------------
class CK_ConfigurationScenario_WorldDialog
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.ResourceNamePicker, desc: "Terrain du scénario (monde parent de la sous-scène)", params: "ent")]
	ResourceName m_sMonde;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Annuler")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Continuer", true)]
	protected bool ButtonOK()
	{
		return true;
	}
}

//------------------------------------------------------------------------------------------------
class CK_ConfigurationScenario_OperationDialog
{
	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Nom de l'opération : nom du scénario (mission header) et titre du briefing CK")]
	string m_sNom;

	[Attribute(defvalue: "0", uiwidget: UIWidgets.Date, desc: "Date de l'opération : nom du mod et du monde (CK_Op_année_mois_jour)")]
	int m_iDate;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Où se passe l'opération : ville et/ou pays (intro de la mission). Vide : Quelque part dans le monde")]
	string m_sLieu;

	[Attribute(defvalue: "", uiwidget: UIWidgets.EditBox, desc: "Scénaristes, séparés par des virgules (auteur du mission header). Vide : Commando Kieffer")]
	string m_sScenaristes;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Annuler")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Continuer", true)]
	protected bool ButtonOK()
	{
		return true;
	}
}

//------------------------------------------------------------------------------------------------
class CK_ConfigurationScenario_ConfirmDialog
{
	static const int RESULT_EDIT = 1;
	static const int RESULT_OK = 2;

	int m_iResult;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Annuler")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Modifier")]
	protected bool ButtonEdit()
	{
		m_iResult = RESULT_EDIT;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Valider", true)]
	protected bool ButtonOK()
	{
		m_iResult = RESULT_OK;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
class CK_ConfigurationScenario_ResumeDialog
{
	static const int RESULT_RESTART = 1;
	static const int RESULT_CONTINUE = 2;

	int m_iResult;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Annuler")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Recommencer")]
	protected bool ButtonRestart()
	{
		m_iResult = RESULT_RESTART;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Continuer", true)]
	protected bool ButtonContinue()
	{
		m_iResult = RESULT_CONTINUE;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
class CK_ConfigurationScenario_RestartDialog
{
	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Plus tard")]
	protected bool ButtonLater()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Redémarrer Workbench", true)]
	protected bool ButtonRestart()
	{
		return true;
	}
}

//------------------------------------------------------------------------------------------------
class CK_ConfigurationScenario_GameModeDialog
{
	static const int RESULT_RETRY = 1;
	static const int RESULT_CONTINUE = 2;

	int m_iResult;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Annuler")]
	protected bool ButtonCancel()
	{
		return false;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Continuer sans")]
	protected bool ButtonContinue()
	{
		m_iResult = RESULT_CONTINUE;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Relancer Game Mode Setup", true)]
	protected bool ButtonRetry()
	{
		m_iResult = RESULT_RETRY;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
class CK_ConfigurationScenario_DoneDialog
{
	static const int RESULT_CLOSE = 1;
	static const int RESULT_OPEN_BRIEFING = 2;

	int m_iResult;

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Fermer")]
	protected bool ButtonClose()
	{
		m_iResult = RESULT_CLOSE;
		return true;
	}

	//------------------------------------------------------------------------------------------------
	[ButtonAttribute("Ouvrir le briefing", true)]
	protected bool ButtonOpenBriefing()
	{
		m_iResult = RESULT_OPEN_BRIEFING;
		return true;
	}
}

//------------------------------------------------------------------------------------------------
//! Default loadout of the CK menu written by the plugin
class CK_ConfigurationScenarioLoadout
{
	string m_sName;
	string m_sDescription;
	string m_sCode;

	//------------------------------------------------------------------------------------------------
	void CK_ConfigurationScenarioLoadout(string name, string description, string code)
	{
		m_sName = name;
		m_sDescription = description;
		m_sCode = code;
	}
}

#endif // WORKBENCH
