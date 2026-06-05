using System;
using System.IO;
using System.Text;
using System.Reflection;
using System.Collections;
using System.Globalization;
using UnityEngine;

namespace lc_manager_sdk
{
    public class Loader
    {
        public static void Init()
        {
            try
            {
                if (GameObject.Find("lc_manager_hook") == (object)null)
                {
                    GameObject hook = new GameObject("lc_manager_hook");
                    hook.AddComponent<DataPipeline>();
                    UnityEngine.Object.DontDestroyOnLoad(hook);
                }
            }
            catch (Exception ex)
            {
                DataPipeline.WriteBootstrapError(ex);
            }
        }

        public static void Unload()
        {
            GameObject hook = GameObject.Find("lc_manager_hook");
            if (hook != (object)null) UnityEngine.Object.Destroy(hook);
        }
    }

    public class DataPipeline : MonoBehaviour
    {
        private string dataPath;

        public static void WriteBootstrapError(Exception ex)
        {
            try
            {
                string targetDir = GetTargetDataDir();
                if (!Directory.Exists(targetDir)) Directory.CreateDirectory(targetDir);
                File.WriteAllText(
                    Path.Combine(targetDir, "live_data.json"),
                    "{\n  \"status\": \"ERROR\",\n  \"message\": \"" + EscapeStatic(ex.Message) + "\"\n}");
            }
            catch { }
        }

        public void Start()
        {
            try
            {
                string targetDir = GetTargetDataDir();
                if (!Directory.Exists(targetDir)) Directory.CreateDirectory(targetDir);
                dataPath = Path.Combine(targetDir, "live_data.json");
                DumpData();
            }
            catch (Exception ex)
            {
                WriteBootstrapError(ex);
            }
        }

        private bool superFastForward = false;
        private float tt15Speed = 5.0f;
        private float tt2Speed = 10.0f;

        public void Update()
        {
            if (Time.frameCount % 60 == 0)
            {
                ReadCommands();
                DumpData();
            }
        }

        public void LateUpdate()
        {
            if (superFastForward)
            {
                if (GameManager.currentGameManager != (object)null)
                {
                    if (GameManager.currentGameManager.gameSpeedLevel == 2)
                    {
                        Time.timeScale = tt15Speed;
                        Time.fixedDeltaTime = 0.02f * tt15Speed;
                    }
                    else if (GameManager.currentGameManager.gameSpeedLevel == 3)
                    {
                        Time.timeScale = tt2Speed;
                        Time.fixedDeltaTime = 0.02f * tt2Speed;
                    }
                }
            }
        }

        private void ReadCommands()
        {
            try
            {
                string targetDir = GetTargetDataDir();
                string cmdsPath = Path.Combine(targetDir, "commands.json");
                if (File.Exists(cmdsPath))
                {
                    string json = File.ReadAllText(cmdsPath);
                    superFastForward = json.Contains("\"superFastForward\": true");
                    
                    int tt15Idx = json.IndexOf("\"tt15Speed\"");
                    if (tt15Idx >= 0)
                    {
                        int colon = json.IndexOf(':', tt15Idx);
                        if (colon >= 0)
                        {
                            int comma = json.IndexOf(',', colon);
                            int endBrace = json.IndexOf('}', colon);
                            int end = (comma >= 0 && comma < endBrace) ? comma : endBrace;
                            if (end > colon)
                            {
                                string val = json.Substring(colon + 1, end - colon - 1).Trim();
                                float.TryParse(val, NumberStyles.Any, CultureInfo.InvariantCulture, out tt15Speed);
                            }
                        }
                    }

                    int tt2Idx = json.IndexOf("\"tt2Speed\"");
                    if (tt2Idx >= 0)
                    {
                        int colon = json.IndexOf(':', tt2Idx);
                        if (colon >= 0)
                        {
                            int comma = json.IndexOf(',', colon);
                            int endBrace = json.IndexOf('}', colon);
                            int end = (comma >= 0 && comma < endBrace) ? comma : endBrace;
                            if (end > colon)
                            {
                                string val = json.Substring(colon + 1, end - colon - 1).Trim();
                                float.TryParse(val, NumberStyles.Any, CultureInfo.InvariantCulture, out tt2Speed);
                            }
                        }
                    }
                    
                    int idx = json.IndexOf("\"targetDir\"");
                    if (idx >= 0)
                    {
                        int start = json.IndexOf('"', idx + 11);
                        if (start >= 0)
                        {
                            int end = json.IndexOf('"', start + 1);
                            if (end >= 0)
                            {
                                string dir = json.Substring(start + 1, end - start - 1).Replace("\\\\", "\\");
                                cmakePath = Path.Combine(dir, "live_data.json");
                            }
                        }
                    }
                }
            }
            catch { }
        }

        private void ApplySuperFastForward()
        {
            try
            {
                object gameManager = GetSingleton("GameManager");
                if (gameManager != (object)null)
                {
                    object state = FirstValue(gameManager, "state", "gameState", "currentGameMode");
                    if (state != (object)null && state.ToString() == "PLAYING")
                    {
                        if (Time.timeScale > 0f && Time.timeScale < 10f)
                        {
                            Time.timeScale = 10f;
                            Time.fixedDeltaTime = 0.08f;
                        }
                    }
                }
            }
            catch { }
        }

        private static string GetTargetDataDir()
        {
            string localAppData = Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            if (string.IsNullOrEmpty(localAppData))
            {
                localAppData = Environment.GetEnvironmentVariable("USERPROFILE") + @"\AppData\Local";
            }
            string appDataDir = Directory.GetParent(localAppData).FullName;
            return Path.Combine(appDataDir, @"LocalLow\zigisoftware\lc_manager\data");
        }
        private string cmakePath = "";

        private void EnsureDataPath()
        {
            if (!string.IsNullOrEmpty(dataPath)) return;
            string targetDir = GetTargetDataDir();
            if (!Directory.Exists(targetDir)) Directory.CreateDirectory(targetDir);
            dataPath = Path.Combine(targetDir, "live_data.json");
        }

        private void WriteData(string text)
        {
            EnsureDataPath();
            try { File.WriteAllText(dataPath, text); } catch { }
            
            if (!string.IsNullOrEmpty(cmakePath))
            {
                try { File.WriteAllText(cmakePath, text); } catch { }
            }
        }

        private static string EscapeStatic(string s)
        {
            if (string.IsNullOrEmpty(s)) return "";
            return s.Replace("\\", "\\\\").Replace("\"", "\\\"").Replace("\n", " ").Replace("\r", "");
        }

        private string Escape(string s)
        {
            return EscapeStatic(s);
        }

        private string JsonValue(object value)
        {
            if (value == (object)null) return "null";

            if (value is bool)
                return ((bool)value) ? "true" : "false";

            if (value is string || value is char || value.GetType().IsEnum)
                return "\"" + Escape(value.ToString()) + "\"";

            try
            {
                if (value is float || value is double || value is decimal)
                    return Convert.ToDouble(value, CultureInfo.InvariantCulture).ToString("0.###", CultureInfo.InvariantCulture);

                if (value is byte || value is short || value is int || value is long ||
                    value is sbyte || value is ushort || value is uint || value is ulong)
                    return Convert.ToString(value, CultureInfo.InvariantCulture);
            }
            catch { }

            return "\"" + Escape(value.ToString()) + "\"";
        }

        private void AppendIndentedProperty(StringBuilder sb, string indent, string name, object value, bool comma)
        {
            sb.Append(indent);
            sb.Append("\"");
            sb.Append(name);
            sb.Append("\": ");
            sb.Append(JsonValue(value));
            if (comma) sb.Append(",");
            sb.Append("\n");
        }

        private void AppendProperty(StringBuilder sb, string name, object value, bool comma)
        {
            AppendIndentedProperty(sb, "    ", name, value, comma);
        }

        private void AppendTopProperty(StringBuilder sb, string name, object value, bool comma)
        {
            AppendIndentedProperty(sb, "  ", name, value, comma);
        }

        private Type GetGameType(string typeName)
        {
            Type type = Type.GetType(typeName + ", Assembly-CSharp");
            if ((object)type != (object)null) return type;

            Assembly[] assemblies = AppDomain.CurrentDomain.GetAssemblies();
            for (int i = 0; i < assemblies.Length; i++)
            {
                try
                {
                    type = assemblies[i].GetType(typeName);
                    if ((object)type != (object)null) return type;
                }
                catch { }
            }

            return null;
        }

        private object GetValue(object target, string name)
        {
            if (target == (object)null || string.IsNullOrEmpty(name)) return null;

            Type type = target as Type;
            object instance = null;
            if ((object)type == (object)null)
            {
                type = target.GetType();
                instance = target;
            }

            BindingFlags flags = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.FlattenHierarchy;
            flags |= instance == (object)null ? BindingFlags.Static : BindingFlags.Instance | BindingFlags.Static;

            Type searchType = type;
            while ((object)searchType != (object)null)
            {
                try
                {
                    FieldInfo field = searchType.GetField(name, flags | BindingFlags.DeclaredOnly);
                    if (field != (object)null) return field.GetValue(field.IsStatic ? null : instance);
                }
                catch { }

                try
                {
                    PropertyInfo property = searchType.GetProperty(name, flags | BindingFlags.DeclaredOnly);
                    if (property != (object)null && property.GetIndexParameters().Length == 0)
                    {
                        MethodInfo getter = property.GetGetMethod(true);
                        return property.GetValue((getter != (object)null && getter.IsStatic) ? null : instance, null);
                    }
                }
                catch { }

                searchType = searchType.BaseType;
            }

            return null;
        }

        private object Call(object target, string methodName)
        {
            if (target == (object)null || string.IsNullOrEmpty(methodName)) return null;

            Type type = target as Type;
            object instance = null;
            if ((object)type == (object)null)
            {
                type = target.GetType();
                instance = target;
            }

            BindingFlags flags = BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.FlattenHierarchy;
            flags |= instance == (object)null ? BindingFlags.Static : BindingFlags.Instance | BindingFlags.Static;

            try
            {
                MethodInfo[] methods = type.GetMethods(flags);
                for (int i = 0; i < methods.Length; i++)
                {
                    MethodInfo method = methods[i];
                    if (method.Name != methodName || method.GetParameters().Length != 0) continue;
                    if (!method.IsStatic && instance == (object)null) continue;
                    return method.Invoke(method.IsStatic ? null : instance, null);
                }
            }
            catch { }

            return null;
        }

        private object FirstValue(object target, params string[] names)
        {
            for (int i = 0; i < names.Length; i++)
            {
                object value = GetValue(target, names[i]);
                if (value != (object)null) return value;
            }
            return null;
        }

        private object FirstCall(object target, params string[] methodNames)
        {
            for (int i = 0; i < methodNames.Length; i++)
            {
                object value = Call(target, methodNames[i]);
                if (value != (object)null) return value;
            }
            return null;
        }

        private object GetSingleton(string typeName)
        {
            Type type = GetGameType(typeName);
            if ((object)type == (object)null) return null;

            object value = FirstValue(type, "instance", "Instance", "_instance", "s_instance", "instnace");
            if (value != (object)null) return value;

            value = FirstCall(type, "get_instance", "GetInstance");
            if (value != (object)null) return value;

            PropertyInfo prop = type.GetProperty("instance", BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.Static);
            if (prop != (object)null) return prop.GetValue(null, null);

            return null;
        }

        private IEnumerable AsEnumerable(object value)
        {
            if (value == (object)null || value is string) return null;
            return value as IEnumerable;
        }

        private ArrayList ToArrayList(IEnumerable values)
        {
            ArrayList list = new ArrayList();
            if (values == (object)null) return list;

            foreach (object value in values)
            {
                if (value != (object)null) list.Add(value);
            }

            return list;
        }

        private int CountEnumerable(IEnumerable values)
        {
            int count = 0;
            if (values == (object)null) return count;

            foreach (object value in values)
            {
                if (value != (object)null) count++;
            }

            return count;
        }

        private bool ToBool(object value, bool fallback)
        {
            if (value == (object)null) return fallback;
            try { return Convert.ToBoolean(value, CultureInfo.InvariantCulture); } catch { }

            string text = Convert.ToString(value, CultureInfo.InvariantCulture);
            if (string.IsNullOrEmpty(text)) return fallback;
            text = text.ToLowerInvariant();
            if (text == "true" || text == "1" || text == "yes") return true;
            if (text == "false" || text == "0" || text == "no") return false;
            return fallback;
        }

        private object DictionaryValue(object dictionary, object key)
        {
            if (dictionary == (object)null || key == (object)null) return null;

            IDictionary map = dictionary as IDictionary;
            if (map == (object)null) return null;

            try
            {
                if (map.Contains(key)) return map[key];
            }
            catch { }

            try
            {
                int intKey = Convert.ToInt32(key, CultureInfo.InvariantCulture);
                if (map.Contains(intKey)) return map[intKey];
            }
            catch { }

            try
            {
                long longKey = Convert.ToInt64(key, CultureInfo.InvariantCulture);
                if (map.Contains(longKey)) return map[longKey];
            }
            catch { }

            return null;
        }

        private IEnumerable GetModelsFromManager(string managerTypeName, string[] methodNames, string[] fieldNames)
        {
            object manager = GetSingleton(managerTypeName);
            Type managerType = GetGameType(managerTypeName);

            if (manager != (object)null)
            {
                IEnumerable list = AsEnumerable(FirstCall(manager, methodNames));
                if (list != (object)null) return list;

                list = AsEnumerable(FirstValue(manager, fieldNames));
                if (list != (object)null) return list;
            }

            if ((object)managerType != (object)null)
            {
                IEnumerable list = AsEnumerable(FirstCall(managerType, methodNames));
                if (list != (object)null) return list;

                list = AsEnumerable(FirstValue(managerType, fieldNames));
                if (list != (object)null) return list;
            }

            return null;
        }

        private object GetAgentManager()
        {
            return GetSingleton("AgentManager");
        }

        private IEnumerable GetWorkingAgentModels()
        {
            object manager = GetAgentManager();
            IEnumerable list = AsEnumerable(FirstValue(manager, "agentList", "_agentList", "agents", "agentModelList", "list"));
            if (list != (object)null) return list;

            return GetModelsFromManager(
                "AgentManager",
                new string[] { "GetAgentList", "GetAgentModelList", "GetAgentListForcely" },
                new string[] { "agentList", "_agentList", "agents", "agentModelList", "list" });
        }

        private IEnumerable GetSpareAgentModels()
        {
            object manager = GetAgentManager();
            IEnumerable list = AsEnumerable(FirstValue(manager, "agentListSpare", "spareAgentList", "_agentListSpare"));
            if (list != (object)null) return list;

            return GetModelsFromManager(
                "AgentManager",
                new string[] { "GetAgentSpareList", "GetSpareAgentList" },
                new string[] { "agentListSpare", "spareAgentList", "_agentListSpare" });
        }

        private IEnumerable GetInventoryEquipmentModels()
        {
            return GetModelsFromManager(
                "InventoryModel",
                new string[] { "GetEquipmentList", "GetEquipList", "GetAllEquipmentList" },
                new string[] { "_equipList", "equipList", "equipmentList", "list" });
        }

        private IEnumerable GetCreatureModels()
        {
            return GetModelsFromManager(
                "CreatureManager",
                new string[] { "GetCreatureList", "GetCreatureDataList" },
                new string[] { "creatureList", "_creatureList", "creatures", "creatureModelList", "list" });
        }

        private object GetNestedValue(object target, string ownerName, string nestedName)
        {
            object owner = GetValue(target, ownerName);
            return owner == (object)null ? null : GetValue(owner, nestedName);
        }

        private object GetAgentName(object agent)
        {
            object directName = FirstValue(agent, "name", "_name");
            if (directName != (object)null) return directName;

            object agentName = FirstValue(agent, "_agentName", "agentName");
            return FirstValue(agentName, "name", "nameString", "id");
        }

        private object GetEquipmentMeta(object equipment)
        {
            return FirstValue(equipment, "metaInfo", "meta", "info");
        }

        private object GetEquipmentTypeId(object equipment)
        {
            object metaInfo = GetEquipmentMeta(equipment);
            return FirstValue(metaInfo, "id", "weaponId", "armorId");
        }

        private object GetEquipmentOwner(object equipment)
        {
            return FirstValue(equipment, "_owner", "owner");
        }

        private object GetUnitEquipment(object unit)
        {
            return FirstValue(unit, "_equipment", "equipment", "equipSpace");
        }

        private void AppendEquipmentObject(StringBuilder sb, object equipment, string indent, string source, object displayState, object lockState)
        {
            if (equipment == (object)null)
            {
                sb.Append("null");
                return;
            }

            object metaInfo = GetEquipmentMeta(equipment);
            object typeId = GetEquipmentTypeId(equipment);
            object owner = GetEquipmentOwner(equipment);

            sb.Append("{\n");
            AppendIndentedProperty(sb, indent + "  ", "instanceId", FirstValue(equipment, "instanceId", "id"), true);
            AppendIndentedProperty(sb, indent + "  ", "typeId", typeId, true);
            AppendIndentedProperty(sb, indent + "  ", "kind", FirstValue(metaInfo, "type"), true);
            AppendIndentedProperty(sb, indent + "  ", "grade", FirstValue(metaInfo, "grade"), true);
            AppendIndentedProperty(sb, indent + "  ", "code", FirstValue(metaInfo, "no", "code", "sprite", "icon"), true);
            AppendIndentedProperty(sb, indent + "  ", "attachPos", FirstValue(metaInfo, "attachPos"), true);
            AppendIndentedProperty(sb, indent + "  ", "attachType", FirstValue(metaInfo, "attachType"), true);
            AppendIndentedProperty(sb, indent + "  ", "source", source, true);
            AppendIndentedProperty(sb, indent + "  ", "displayed", displayState, true);
            AppendIndentedProperty(sb, indent + "  ", "lockState", lockState, true);
            AppendIndentedProperty(sb, indent + "  ", "ownerId", FirstValue(owner, "instanceId", "id"), true);
            AppendIndentedProperty(sb, indent + "  ", "ownerName", GetAgentName(owner), true);
            AppendIndentedProperty(sb, indent + "  ", "ownerSefira", FirstValue(owner, "currentSefira", "_currentSefira", "currentSefiraEnum", "sefira"), false);
            sb.Append(indent);
            sb.Append("}");
        }

        private void AppendGiftListItems(StringBuilder sb, IEnumerable gifts, string source, object displayStateMap, object lockStateMap, ref bool first)
        {
            if (gifts == (object)null) return;

            foreach (object gift in gifts)
            {
                if (gift == (object)null) continue;

                object typeId = GetEquipmentTypeId(gift);
                object displayState = DictionaryValue(displayStateMap, typeId);
                object lockState = DictionaryValue(lockStateMap, typeId);

                if (!first) sb.Append(",\n");
                sb.Append("        ");
                AppendEquipmentObject(sb, gift, "        ", source, displayState, lockState);
                first = false;
            }
        }

        private void AppendGiftArray(StringBuilder sb, string indent, string name, object giftSpace, bool comma)
        {
            IEnumerable addedGifts = AsEnumerable(FirstValue(giftSpace, "addedGifts"));
            IEnumerable replacedGifts = AsEnumerable(FirstValue(giftSpace, "replacedGifts"));
            object displayState = FirstValue(giftSpace, "displayState");
            object lockState = FirstValue(giftSpace, "lockState");

            sb.Append(indent);
            sb.Append("\"");
            sb.Append(name);
            sb.Append("\": [\n");

            bool first = true;
            AppendGiftListItems(sb, addedGifts, "added", displayState, lockState, ref first);
            AppendGiftListItems(sb, replacedGifts, "replaced", displayState, lockState, ref first);

            sb.Append("\n");
            sb.Append(indent);
            sb.Append("]");
            if (comma) sb.Append(",");
            sb.Append("\n");
        }

        private void AppendAgentEquipment(StringBuilder sb, object agent)
        {
            object equipment = GetUnitEquipment(agent);
            object weapon = FirstValue(equipment, "weapon");
            object armor = FirstValue(equipment, "armor");
            object giftSpace = FirstValue(equipment, "gifts");
            IEnumerable addedGifts = AsEnumerable(FirstValue(giftSpace, "addedGifts"));
            IEnumerable replacedGifts = AsEnumerable(FirstValue(giftSpace, "replacedGifts"));
            int addedGiftCount = CountEnumerable(addedGifts);
            int replacedGiftCount = CountEnumerable(replacedGifts);

            sb.Append("    \"equipment\": {\n");
            AppendIndentedProperty(sb, "      ", "giftCount", addedGiftCount + replacedGiftCount, true);
            AppendIndentedProperty(sb, "      ", "addedGiftCount", addedGiftCount, true);
            AppendIndentedProperty(sb, "      ", "replacedGiftCount", replacedGiftCount, true);
            sb.Append("      \"weapon\": ");
            AppendEquipmentObject(sb, weapon, "      ", null, null, null);
            sb.Append(",\n");
            sb.Append("      \"armor\": ");
            AppendEquipmentObject(sb, armor, "      ", null, null, null);
            sb.Append(",\n");
            AppendGiftArray(sb, "      ", "gifts", giftSpace, false);
            sb.Append("    }\n");
        }

        private object GetWorkTarget(object work)
        {
            object target = FirstValue(work, "target", "creature", "creatureModel", "model", "abnormality");
            if (target != (object)null) return target;
            return FirstValue(work, "_target", "_creature", "_creatureModel");
        }

        private void AppendAgentWork(StringBuilder sb, object agent)
        {
            object work = FirstValue(agent, "currentWork", "work", "currentSkill", "skill", "task", "_currentSkill");
            object target = GetWorkTarget(work);
            if (target == (object)null) target = FirstValue(agent, "target", "targetCreature", "workingCreature", "currentCreature");
            object targetMeta = FirstValue(target, "metaInfo", "meta");

            sb.Append("    \"currentWork\": {\n");
            AppendIndentedProperty(sb, "      ", "category", FirstValue(work, "category", "workType", "type", "name"), true);
            AppendIndentedProperty(sb, "      ", "state", FirstValue(work, "state", "status"), true);
            AppendIndentedProperty(sb, "      ", "successRate", FirstValue(work, "successRate", "successProb", "probability"), true);
            AppendIndentedProperty(sb, "      ", "elapsedTime", FirstValue(work, "elapsedTime", "progressTime", "elapsed"), true);
            AppendIndentedProperty(sb, "      ", "remainTime", FirstValue(work, "remainTime", "remainingTime", "timeLeft"), true);
            AppendIndentedProperty(sb, "      ", "totalTime", FirstValue(work, "totalTime", "workTime", "duration", "maxTime"), true);
            AppendIndentedProperty(sb, "      ", "abnormalityId", FirstValue(target, "metadataId", "metaId", "id") ?? GetValue(targetMeta, "id"), true);
            AppendIndentedProperty(sb, "      ", "abnormalityName", GetValue(targetMeta, "name"), true);
            AppendIndentedProperty(sb, "      ", "abnormalityCode", FirstValue(target, "codeId", "code") ?? GetValue(targetMeta, "codeId"), false);
            sb.Append("    },\n");
        }

        private void AppendAgent(StringBuilder sb, object agent, string deployment)
        {
            object primaryStat = FirstValue(agent, "primaryStat", "baseStat", "stat");
            object primaryStatExp = FirstValue(agent, "primaryStatExp");

            sb.Append("    {\n");
            AppendProperty(sb, "id", FirstValue(agent, "instanceId", "id"), true);
            AppendProperty(sb, "name", GetAgentName(agent), true);
            AppendProperty(sb, "deployment", deployment, true);
            AppendProperty(sb, "level", FirstValue(agent, "level", "grade"), true);
            AppendProperty(sb, "currentSefira", FirstValue(agent, "currentSefira", "_currentSefira", "currentSefiraEnum", "sefira"), true);
            AppendProperty(sb, "lastServiceSefira", FirstValue(agent, "lastServiceSefira"), true);
            AppendProperty(sb, "continuousServiceDay", FirstValue(agent, "continuousServiceDay"), true);
            AppendProperty(sb, "isAce", FirstValue(agent, "isAce"), true);
            AppendProperty(sb, "fortitude", FirstValue(agent, "originFortitudeStat", "fortitude") ?? GetValue(primaryStat, "hp"), true);
            AppendProperty(sb, "prudence", FirstValue(agent, "originPrudenceStat", "prudence") ?? GetValue(primaryStat, "mental"), true);
            AppendProperty(sb, "temperance", FirstValue(agent, "originTemperanceStat", "temperance") ?? GetValue(primaryStat, "work"), true);
            AppendProperty(sb, "justice", FirstValue(agent, "originJusticeStat", "justice") ?? GetValue(primaryStat, "battle"), true);
            AppendProperty(sb, "hp", FirstValue(agent, "hp", "Hp") ?? GetValue(primaryStat, "hp"), true);
            AppendProperty(sb, "sp", FirstValue(agent, "mental", "Mental", "sp") ?? GetValue(primaryStat, "mental"), true);
            AppendProperty(sb, "maxHp", FirstValue(agent, "baseMaxHp", "maxHp"), true);
            AppendProperty(sb, "maxSp", FirstValue(agent, "baseMaxMental", "maxMental"), true);
            
            Type statUtils = GetGameType("StatUtils");
            MethodInfo statM = (object)statUtils != (object)null ? statUtils.GetMethod("GetStatEXPValue", BindingFlags.Static | BindingFlags.Public) : null;
            if (statM != (object)null)
            {
                AppendProperty(sb, "expHp", statM.Invoke(null, new object[] { agent, 0 }), true);
                AppendProperty(sb, "expMental", statM.Invoke(null, new object[] { agent, 1 }), true);
                AppendProperty(sb, "expWork", statM.Invoke(null, new object[] { agent, 2 }), true);
                AppendProperty(sb, "expBattle", statM.Invoke(null, new object[] { agent, 3 }), true);
            }
            else
            {
                object expHp = GetValue(primaryStatExp, "hp") ?? FirstValue(agent, "expHp");
                object expMental = GetValue(primaryStatExp, "mental") ?? FirstValue(agent, "expMental");
                object expWork = GetValue(primaryStatExp, "work") ?? FirstValue(agent, "expWork");
                object expBattle = GetValue(primaryStatExp, "battle") ?? FirstValue(agent, "expBattle");
                AppendProperty(sb, "expHp", expHp, true);
                AppendProperty(sb, "expMental", expMental, true);
                AppendProperty(sb, "expWork", expWork, true);
                AppendProperty(sb, "expBattle", expBattle, true);
            }
            AppendAgentWork(sb, agent);
            AppendAgentEquipment(sb, agent);
            sb.Append("    }");
        }

        private void AppendAgentArray(StringBuilder sb, string name, IEnumerable agents, string deployment, bool comma)
        {
            sb.Append("  \"");
            sb.Append(name);
            sb.Append("\": [\n");

            bool first = true;
            if (agents != (object)null)
            {
                foreach (object agent in agents)
                {
                    if (agent == (object)null) continue;
                    if (!first) sb.Append(",\n");
                    AppendAgent(sb, agent, deployment);
                    first = false;
                }
            }

            sb.Append("\n  ]");
            if (comma) sb.Append(",");
            sb.Append("\n");
        }

        private void AppendCreature(StringBuilder sb, object creature)
        {
            object metaInfo = FirstValue(creature, "metaInfo", "meta");
            object observeInfo = FirstValue(creature, "observeInfo", "observationInfo", "observeData", "researchInfo");
            object metadataId = FirstValue(creature, "metadataId", "metaId") ?? GetValue(metaInfo, "id");
            object code = FirstValue(creature, "codeId", "code") ?? GetValue(metaInfo, "codeId");

            sb.Append("    {\n");
            AppendProperty(sb, "metadataId", metadataId, true);
            AppendProperty(sb, "instanceId", FirstValue(creature, "instanceId", "id"), true);
            AppendProperty(sb, "code", code, true);
            AppendProperty(sb, "name", GetValue(metaInfo, "name"), true);
            AppendProperty(sb, "sefiraNum", FirstValue(creature, "sefiraNum", "sefira"), true);
            AppendProperty(sb, "baseMaxHp", FirstValue(creature, "baseMaxHp", "maxHp"), true);
            AppendProperty(sb, "hp", FirstValue(creature, "hp"), true);
            AppendProperty(sb, "state", FirstValue(creature, "state", "_state", "currentState"), true);
            AppendProperty(sb, "observationLevel", FirstValue(creature, "observationLevel", "observeLevel", "observe", "researchLevel") ?? FirstValue(observeInfo, "level", "observeLevel", "researchLevel"), true);
            AppendProperty(sb, "observationPercent", FirstValue(creature, "observationPercent", "researchPercent", "completion") ?? FirstValue(observeInfo, "percent", "completion"), true);
            AppendProperty(sb, "peBox", FirstValue(observeInfo, "cubeNum", "peBoxCount", "pe", "_peBox"), true);
            bool isMeltdown = Convert.ToBoolean(FirstValue(creature, "isOverloaded", "isMeltdown", "_isMeltdown") ?? false);
            AppendProperty(sb, "isMeltdown", isMeltdown, true);
            if (isMeltdown) {
                float maxTime = Convert.ToSingle(FirstValue(creature, "currentOverloadMaxTime") ?? 60.0f);
                float timer = Convert.ToSingle(FirstValue(creature, "overloadTimer") ?? 0.0f);
                AppendProperty(sb, "meltdownTimer", maxTime - timer, true);
            } else {
                AppendProperty(sb, "meltdownTimer", 0, true);
            }
            AppendProperty(sb, "workCount", FirstValue(creature, "workCount", "totalWorkCount") ?? FirstValue(observeInfo, "workCount", "totalWorkCount"), true);
            
            object modelObj = GetValue(creature, "model");
            object qc = FirstValue(creature, "qliphothCounter", "qliphoth", "_qliphothCounter", "outbreakCount");
            if (qc == (object)null && modelObj != (object)null) qc = FirstValue(modelObj, "qliphothCounter");
            AppendProperty(sb, "qliphothCounter", qc, true);
            
            AppendProperty(sb, "worker", GetAgentName(FirstValue(creature, "worker", "currentWorker", "workerModel", "_worker")), true);
            AppendProperty(sb, "totalEnergy", FirstValue(creature, "totalEnergy", "energy", "collectedEnergy"), true);

            object defense = FirstValue(creature, "defense", "defenseInfo");
            if (defense == (object)null && modelObj != (object)null) defense = FirstValue(modelObj, "defense", "defenseInfo");
            
            if (defense != (object)null) {
                AppendProperty(sb, "defR", FirstValue(defense, "R", "r"), true);
                AppendProperty(sb, "defW", FirstValue(defense, "W", "w"), true);
                AppendProperty(sb, "defB", FirstValue(defense, "B", "b"), true);
                AppendProperty(sb, "defP", FirstValue(defense, "P", "p"), false);
            } else {
                AppendProperty(sb, "defR", 1.0, true);
                AppendProperty(sb, "defW", 1.0, true);
                AppendProperty(sb, "defB", 1.0, true);
                AppendProperty(sb, "defP", 1.0, false);
            }
            sb.Append("\n    }");
        }

        private void AppendOrdealEntry(StringBuilder sb, object entry)
        {
            object meta = FirstValue(entry, "meta", "metaInfo", "info", "data");

            sb.Append("{\n");
            AppendIndentedProperty(sb, "      ", "name", FirstValue(entry, "name", "ordealName", "displayName") ?? FirstValue(meta, "name", "displayName"), true);
            AppendIndentedProperty(sb, "      ", "level", FirstValue(entry, "level", "ordealLevel") ?? FirstValue(meta, "level", "ordealLevel"), true);
            AppendIndentedProperty(sb, "      ", "color", FirstValue(entry, "color", "ordealColor") ?? FirstValue(meta, "color", "ordealColor"), true);
            AppendIndentedProperty(sb, "      ", "type", FirstValue(entry, "type", "kind", "ordealType") ?? FirstValue(meta, "type", "kind", "ordealType"), true);
            AppendIndentedProperty(sb, "      ", "trigger", FirstValue(entry, "trigger", "time", "meltdown", "qliphothLevel"), false);
            sb.Append("    }");
        }

        private void AppendOrdealQueue(StringBuilder sb, IEnumerable queue)
        {
            sb.Append("    \"queue\": [\n");

            bool first = true;
            if (queue != (object)null)
            {
                foreach (object entry in queue)
                {
                    if (entry == (object)null) continue;
                    if (!first) sb.Append(",\n");
                    sb.Append("    ");
                    AppendOrdealEntry(sb, entry);
                    first = false;
                }
            }

            sb.Append("\n    ]\n");
        }

        private void AppendOrdeals(StringBuilder sb)
        {
            try
            {
                object ordealManager = GetSingleton("OrdealManager");
                if (ordealManager != (object)null)
                {
                    FieldInfo field = ordealManager.GetType().GetField("_ordealList", BindingFlags.Instance | BindingFlags.NonPublic);
                    if (field != (object)null)
                    {
                        IList ordealList = (IList)field.GetValue(ordealManager);
                        sb.Append("  \"ordeals\": [\n");
                        for (int i = 0; i < ordealList.Count; i++)
                        {
                            object ordeal = ordealList[i];
                            string ordealType = ordeal.GetType().Name;
                            string name = "Unknown";
                            
                            FieldInfo nameField = ordeal.GetType().GetField("_ordealName", BindingFlags.Instance | BindingFlags.NonPublic);
                            if (nameField != (object)null)
                            {
                                name = (string)nameField.GetValue(ordeal);
                            }
                            
                            sb.Append("    {\n");
                            sb.Append("      \"type\": \"" + Escape(ordealType) + "\",\n");
                            sb.Append("      \"name\": \"" + Escape(name) + "\"\n");
                            sb.Append("    }");
                            if (i < ordealList.Count - 1) sb.Append(",");
                            sb.Append("\n");
                        }
                        sb.Append("  ],\n");
                    }
                }
            }
            catch { }
        }

        private void AppendOrdeal(StringBuilder sb)
        {
            object ordeal = GetSingleton("OrdealManager");
            object queue = FirstValue(ordeal, "_ordealQueue", "ordealQueue");

            sb.Append("  \"ordeal\": {\n");
            AppendProperty(sb, "currentLevel", FirstValue(ordeal, "_currentOrdealLevel", "currentOrdealLevel"), true);
            AppendProperty(sb, "elapsedTime", FirstValue(ordeal, "_elapsedTime", "elapsedTime"), true);
            AppendProperty(sb, "remainTime", FirstValue(ordeal, "_remainTime", "remainTime"), true);
            AppendProperty(sb, "queueCount", GetValue(queue, "Count"), true);
            AppendOrdealQueue(sb, AsEnumerable(queue));
            sb.Append("  }\n");
        }

        private void AppendExtractionChoices(StringBuilder sb)
        {
            try
            {
                object selectUI = GetSingleton("CreatureSelectUI");
                bool isExtracting = false;
                
                if (selectUI != (object)null)
                {
                    object enabledProp = FirstValue(selectUI, "IsEnabled");
                    if (enabledProp != (object)null) isExtracting = Convert.ToBoolean(enabledProp);
                    else
                    {
                        object go = FirstValue(selectUI, "gameObject");
                        if (go != (object)null)
                        {
                            object act = FirstCall(go, "get_activeInHierarchy");
                            if (act == (object)null) act = FirstValue(go, "activeInHierarchy");
                            if (act != (object)null) isExtracting = Convert.ToBoolean(act);
                        }
                    }
                }
                
                sb.Append("  \"isExtracting\": " + (isExtracting ? "true" : "false") + ",\n");
                sb.Append("  \"extractionChoices\": [\n");
                
                if (selectUI != (object)null && isExtracting)
                {
                    object units = FirstValue(selectUI, "Units", "units");
                    if (units != (object)null)
                    {
                        Array unitArray = units as Array;
                        if (unitArray != (object)null)
                        {
                            bool first = true;
                            for (int i = 0; i < unitArray.Length; i++)
                            {
                                object unit = unitArray.GetValue(i);
                                if (unit == (object)null) continue;
                                
                                object creatureIdObj = FirstValue(unit, "CreatureID", "creatureID", "creatureId", "_creatureId");
                                if (creatureIdObj != (object)null)
                                {
                                    long creatureId = Convert.ToInt64(creatureIdObj);
                                    if (creatureId > 0)
                                    {
                                        if (!first) sb.Append(",\n");
                                        sb.Append("    " + Convert.ToString(creatureId, CultureInfo.InvariantCulture));
                                        first = false;
                                    }
                                }
                            }
                        }
                    }
                }
            }
            catch {
                sb.Append("  \"isExtracting\": false,\n");
                sb.Append("  \"extractionChoices\": [\n");
            }
            sb.Append("\n  ]\n");
        }

        private void AppendInventoryCounts(StringBuilder sb, ArrayList equipmentItems)
        {
            Hashtable counts = new Hashtable();
            ArrayList keys = new ArrayList();

            for (int i = 0; i < equipmentItems.Count; i++)
            {
                object typeId = GetEquipmentTypeId(equipmentItems[i]);
                string key = typeId == (object)null ? "unknown" : Convert.ToString(typeId, CultureInfo.InvariantCulture);
                if (!counts.ContainsKey(key))
                {
                    counts[key] = 0;
                    keys.Add(key);
                }
                counts[key] = ((int)counts[key]) + 1;
            }

            sb.Append("    \"countsByTypeId\": {\n");
            for (int i = 0; i < keys.Count; i++)
            {
                string key = (string)keys[i];
                sb.Append("      \"");
                sb.Append(Escape(key));
                sb.Append("\": ");
                sb.Append(Convert.ToString(counts[key], CultureInfo.InvariantCulture));
                if (i < keys.Count - 1) sb.Append(",");
                sb.Append("\n");
            }
            sb.Append("    },\n");
        }

        private void AppendEquipmentItemArray(StringBuilder sb, ArrayList equipmentItems)
        {
            sb.Append("    \"items\": [\n");
            for (int i = 0; i < equipmentItems.Count; i++)
            {
                if (i > 0) sb.Append(",\n");
                sb.Append("      ");
                AppendEquipmentObject(sb, equipmentItems[i], "      ", null, null, null);
            }
            sb.Append("\n    ]\n");
        }

        private void AppendInventory(StringBuilder sb)
        {
            ArrayList equipmentItems = ToArrayList(GetInventoryEquipmentModels());

            sb.Append("  \"inventory\": {\n");
            AppendIndentedProperty(sb, "    ", "totalItems", equipmentItems.Count, true);
            AppendInventoryCounts(sb, equipmentItems);
            AppendEquipmentItemArray(sb, equipmentItems);
            sb.Append("  },\n");
        }

        private IEnumerable GetOfficerModels()
        {
            return GetModelsFromManager(
                "OfficerManager",
                new string[] { "GetOfficerList", "GetOfficerModelList", "GetClerkList", "GetClerkModelList" },
                new string[] { "officerList", "_officerList", "clerkList", "_clerkList", "list" });
        }

        private void AppendClerks(StringBuilder sb)
        {
            Hashtable total = new Hashtable();
            Hashtable alive = new Hashtable();
            ArrayList keys = new ArrayList();
            IEnumerable officers = GetOfficerModels();

            if (officers != (object)null)
            {
                foreach (object officer in officers)
                {
                    if (officer == (object)null) continue;

                    object departmentValue = FirstValue(officer, "currentSefira", "_currentSefira", "currentSefiraEnum", "sefira", "department", "sefiraNum");
                    string department = departmentValue == (object)null ? "unknown" : Convert.ToString(departmentValue, CultureInfo.InvariantCulture);
                    if (!total.ContainsKey(department))
                    {
                        total[department] = 0;
                        alive[department] = 0;
                        keys.Add(department);
                    }

                    total[department] = ((int)total[department]) + 1;

                    bool isAlive = ToBool(FirstValue(officer, "isAlive", "alive"), true);
                    bool isDead = ToBool(FirstValue(officer, "isDead", "dead", "isKilled", "killed"), false);
                    object hpObj = FirstValue(officer, "hp", "Hp");
                    if (hpObj != (object)null)
                    {
                        try { if (Convert.ToDouble(hpObj, CultureInfo.InvariantCulture) <= 0.0) isDead = true; } catch { }
                    }

                    if (isAlive && !isDead) alive[department] = ((int)alive[department]) + 1;
                }
            }

            sb.Append("  \"clerks\": {\n");
            for (int i = 0; i < keys.Count; i++)
            {
                string key = (string)keys[i];
                int totalCount = (int)total[key];
                int aliveCount = (int)alive[key];
                int deadCount = totalCount - aliveCount;

                sb.Append("    \"");
                sb.Append(Escape(key));
                sb.Append("\": {\n");
                AppendIndentedProperty(sb, "      ", "total", totalCount, true);
                AppendIndentedProperty(sb, "      ", "alive", aliveCount, true);
                AppendIndentedProperty(sb, "      ", "dead", deadCount, false);
                sb.Append("    }");
                if (i < keys.Count - 1) sb.Append(",");
                sb.Append("\n");
            }
            sb.Append("  },\n");
        }

        private void DumpWaiting(StringBuilder sb, string reason)
        {
            sb.Append("{\n");
            AppendTopProperty(sb, "status", "WAITING FOR DATA", true);
            AppendTopProperty(sb, "reason", reason, false);
            sb.Append("}");
            WriteData(sb.ToString());
        }

        private void AppendEnergy(StringBuilder sb)
        {
            object energyModel = GetSingleton("EnergyModel");
            object stageTypeInfo = GetSingleton("StageTypeInfo");
            object playerModel = GetSingleton("PlayerModel");
            
            float energy = 0f;
            if (energyModel != (object)null)
            {
                object e = FirstCall(energyModel, "GetEnergy");
                if (e != (object)null) energy = Convert.ToSingle(e);
            }
            
            float need = 0f;
            if (stageTypeInfo != (object)null && playerModel != (object)null)
            {
                object dayObj = FirstCall(playerModel, "GetDay");
                if (dayObj != (object)null)
                {
                    try {
                        MethodInfo method = stageTypeInfo.GetType().GetMethod("GetEnergyNeed", BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.FlattenHierarchy);
                        if (method != (object)null)
                        {
                            object n = method.Invoke(stageTypeInfo, new object[] { dayObj });
                            if (n != (object)null) need = Convert.ToSingle(n);
                        }
                    } catch {}
                }
            }
            
            sb.Append("  \"energyDetails\": {\n");
            AppendIndentedProperty(sb, "    ", "current", energy, true);
            AppendIndentedProperty(sb, "    ", "need", need, false);
            sb.Append("  },\n");
        }

        private void DumpData()
        {
            try
            {
                StringBuilder sb = new StringBuilder();
                object playerModel = GetSingleton("PlayerModel");

                if ((object)playerModel == null || playerModel.Equals(null))
                {
                    WriteData("{\"status\": \"MAIN_MENU\"}");
                    return;
                }

                object completedDayObj = FirstValue(playerModel, "day", "Day");
                int completedDay = 0;
                try { completedDay = Convert.ToInt32(completedDayObj, CultureInfo.InvariantCulture); } catch { }

                object moneyModel = GetSingleton("MoneyModel");
                object agentManager = GetAgentManager();
                object energyModel = GetSingleton("EnergyModel");
                object energy = FirstValue(energyModel, "energy", "Energy");
                object lob = FirstValue(moneyModel, "money", "Money", "lobPoint", "LobPoint", "point");
                
                object playTime = FirstValue(playerModel, "playTime", "PlayTime");
                if (playTime == (object)null) {
                    object globalGameManager = GetSingleton("GlobalGameManager");
                    playTime = FirstValue(globalGameManager, "playTime", "PlayTime");
                }
                if (playTime == (object)null) {
                    object gameManager = GetSingleton("GameManager");
                    playTime = FirstValue(gameManager, "playTime", "PlayTime");
                }

                sb.Append("{\n");
                AppendTopProperty(sb, "status", "ONLINE", true);
                AppendTopProperty(sb, "day", completedDay + 1, true);
                AppendTopProperty(sb, "lob", lob, true);
                AppendTopProperty(sb, "playTime", playTime, true);
                AppendOrdeals(sb);
                AppendTopProperty(sb, "energy", energy, true);
                AppendTopProperty(sb, "nextInstId", FirstValue(agentManager, "nextInstId") ?? FirstValue(playerModel, "nextInstId"), true);

                AppendAgentArray(sb, "agents", GetWorkingAgentModels(), "working", true);
                AppendAgentArray(sb, "spareAgents", GetSpareAgentModels(), "spare", true);

                sb.Append("  \"abnormalities\": [\n");
                IEnumerable creatures = GetCreatureModels();
                bool first = true;
                if (creatures != (object)null)
                {
                    foreach (object creature in creatures)
                    {
                        if (creature == (object)null) continue;
                        if (!first) sb.Append(",\n");
                        AppendCreature(sb, creature);
                        first = false;
                    }
                }
                sb.Append("\n  ],\n");

                AppendInventory(sb);
                AppendClerks(sb);
                AppendEnergy(sb);
                AppendOrdeal(sb);
                sb.Append(",\n");
                AppendExtractionChoices(sb);
                sb.Append("}");

                WriteData(sb.ToString());
            }
            catch (Exception ex)
            {
                try
                {
                    if (ex.GetType().Name == "MissingReferenceException" || ex.GetType().Name == "NullReferenceException")
                    {
                        WriteData("{\"status\": \"MAIN_MENU\"}");
                    }
                    else
                    {
                        WriteData("{\n  \"status\": \"ERROR\",\n  \"message\": \"" + Escape(ex.Message) + "\",\n  \"stack\": \"" + Escape(ex.StackTrace) + "\"\n}");
                    }
                }
                catch { }
            }
        }
    }
}
