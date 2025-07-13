#ifndef COMS_QUEST_EVENT_TASK_TYPE_H
#define COMS_QUEST_EVENT_TASK_TYPE_H

enum EventTaskType {
    TASK_COLLECT,            // Collect a specific item or set of items
    TASK_KILL,               // Kill a certain number of enemies
    TASK_ACCOMPANY,          // Escort an NPC to a location
    TASK_DISCOVER,           // Discover a specific location
    TASK_DELIVER,            // Deliver an item to an NPC or location
    TASK_RESCUE,             // Rescue an NPC or group of NPCs
    TASK_RETRIEVE,           // Retrieve a stolen or lost item
    TASK_GATHER_INFORMATION, // Gather information or clues
    TASK_TALK,               // Speak to a specific NPC or group of NPCs
    TASK_DEFEND,             // Defend a location or NPC for a certain period of time
    TASK_SURVIVE,            // Survive waves of enemies or a hazardous environment
    TASK_INVESTIGATE,        // Investigate a mystery or event
    TASK_CONSTRUCT,          // Build or repair a structure or object
    TASK_CRAFT,              // Craft a specific item or set of items
    TASK_TRAIN,              // Train a skill or teach an NPC
    TASK_FOLLOW,             // Follow an NPC or creature without being detected
    TASK_SABOTAGE,           // Sabotage an enemy structure or plan
    TASK_CAPTURE,            // Capture or trap an enemy or creature
    TASK_ESCAPE,             // Escape from a location or situation
    TASK_EXPLORE,            // Explore a new area or region
    TASK_NEGOTIATE,          // Negotiate peace or a trade agreement
    TASK_BARGAIN,            // Bargain for a better price or outcome
    TASK_SPY,                // Gather intelligence on enemies or allies
    TASK_INFILTRATE,         // Infiltrate an enemy base or organization
    TASK_ASSASSINATE,        // Assassinate a key target
    TASK_RECON,              // Perform reconnaissance on a location
    TASK_HACK,               // Hack a computer system or device
    TASK_REPAIR,             // Repair a damaged object or system
    TASK_ENCHANT,            // Enchant a weapon, armor, or item
    TASK_BREW,               // Brew a potion or drink
    TASK_FISH,               // Catch a certain type or number of fish
    TASK_HUNT,               // Hunt a specific animal or creature
    TASK_COLLECT_TAX,        // Collect taxes or tribute from NPCs
    TASK_RECRUIT,            // Recruit allies or troops for a cause
    TASK_ESCAPE_PRISON,      // Escape from captivity or imprisonment
    TASK_EXTRACT,            // Extract resources or an NPC from a location
    TASK_SHADOW,             // Shadow an NPC or enemy without being seen
    TASK_FIND_LOST,          // Find a lost or missing NPC or item
    TASK_PLANT,              // Plant seeds or objects in specific locations
    TASK_SELL,               // Sell specific items to merchants or NPCs
    TASK_BUY,                // Purchase specific items or goods
    TASK_PROTECT,            // Protect a caravan or convoy
    TASK_RELOCATE,           // Relocate an NPC or item to a new location
    TASK_RESTORE,            // Restore a broken artifact or relationship
    TASK_SURVEY,             // Survey land or resources
    TASK_SMUGGLE,            // Smuggle contraband past guards or checkpoints
    TASK_INTERROGATE,        // Interrogate an NPC for information
    TASK_INTERCEPT,          // Intercept a message or courier
    TASK_ESCORT,             // Escort an important NPC through dangerous territory
    TASK_MINE,               // Mine specific ores or minerals
    TASK_BOUNTY,             // Collect a bounty on a specific target
    TASK_TRAP,               // Set traps for enemies or animals
    TASK_CONQUER,            // Conquer a specific area or territory
    TASK_SURRENDER,          // Surrender to an enemy or superior
    TASK_NEGLECT,            // Avoid or neglect a responsibility (e.g., a moral choice)
    TASK_SUPPORT,            // Provide support or aid to allies in a conflict
    TASK_DELIVER_MESSAGE,    // Deliver a message or letter to an NPC
    TASK_LAY_SIEGE,          // Lay siege to a fortress or city
    TASK_BREAK_SIEGE,        // Break a siege and liberate a location
    TASK_FORAGE,             // Forage for food, herbs, or supplies
    TASK_SEARCH,             // Search for clues or specific items in an area
    TASK_TAME,               // Tame a wild animal or creature
    TASK_CHARM,              // Charm or persuade an NPC or creature
    TASK_CONVINCE,           // Convince an NPC to take a certain action
    TASK_DEFECT,             // Defect from one faction to another
    TASK_BETRAY,             // Betray an ally or group for personal gain
    TASK_DECRYPT,            // Decrypt a coded message or puzzle
    TASK_PUZZLE,             // Solve a puzzle or riddle
    TASK_RACE,               // Participate in a race or timed event
    TASK_TOURNAMENT,         // Compete in a tournament or competition
    TASK_SCARE,              // Scare off or intimidate an NPC or creature
    TASK_SHOOT,              // Shoot targets or enemies with a ranged weapon
    TASK_USE_ITEM,           // Use a specific item at a location or on an NPC
    TASK_PERSUADE,           // Persuade an NPC to do something or believe something
    TASK_MEDITATE,           // Meditate to gain insights or abilities
    TASK_DUEL,               // Duel an NPC in combat
    TASK_TRIAL,              // Complete a trial or test of skill
    TASK_PREACH,             // Preach to NPCs to gain followers or influence
    TASK_CONVERT,            // Convert NPCs to a religion or cause
    TASK_COLLECT_DEBT,       // Collect a debt from an NPC
    TASK_LIGHT,              // Light torches or beacons
    TASK_EXTINGUISH,         // Extinguish fires or light sources
    TASK_RECONCILE,          // Reconcile a conflict between NPCs or groups
    TASK_DISARM,             // Disarm traps or enemies
    TASK_RELOCATE_VILLAGERS, // Relocate villagers to a safe area
    TASK_CALM,               // Calm a panicked NPC or creature
    TASK_INVOKE_RITUAL,      // Invoke or complete a ritual
    TASK_PERFORM,            // Perform music, dance, or a play for an audience
    TASK_OBTAIN_PERMISSION,  // Obtain permission to enter a location or take an action
    TASK_HIDE,               // Hide from enemies or during a stealth mission
    TASK_CLIMB,              // Climb a mountain, tree, or other structure
    TASK_SWIM,               // Swim across a body of water or retrieve an item from underwater
    TASK_DIVE,               // Dive into deep water or a cave
    TASK_RESIST,             // Resist temptation or a magical effect
    TASK_SNEAK,              // Sneak into a location without being detected
    TASK_SURVEY_TERRITORY,   // Survey territory for strategic purposes
    TASK_COMMUNE,            // Commune with spirits, gods, or nature
    TASK_EXORCISE,           // Exorcise a spirit or demon from a location or person
    TASK_SUMMON,             // Summon a creature, spirit, or other entity
    TASK_DECEIVE,            // Deceive an NPC or group
    TASK_GATHER_ARMY,        // Gather an army or group of followers
    TASK_REPEL_INVASION,     // Repel an invasion or attack
    TASK_HARVEST,            // Harvest crops or resources
    TASK_OPERATE_MACHINE,    // Operate a complex machine or vehicle
    TASK_NEGOTIATE_RANSOM,   // Negotiate the ransom of a captured NPC
    TASK_CONVICT,            // Convict or acquit an NPC in a trial
    TASK_TRACK,              // Track a person, creature, or object
    TASK_CONCEAL,            // Conceal evidence or an object from others
    TASK_APPREHEND,          // Apprehend a criminal or fugitive
    TASK_TEACH,              // Teach an NPC a skill or knowledge
    TASK_CURE,               // Cure a disease or ailment affecting an NPC
    TASK_RECLAIM,            // Reclaim lost or stolen property
    TASK_HUNT_TREASURE,      // Hunt for hidden or buried treasure
    TASK_PROPHESY,           // Deliver a prophecy or prediction
    TASK_ENACT_REVENGE,      // Enact revenge on an enemy or traitor
    TASK_UPHOLD_OATH,        // Uphold an oath or pledge
    TASK_FIND_PATH,          // Find a path through difficult terrain
    TASK_ESCORT_CONVOY,      // Escort a convoy through dangerous territory
    TASK_INTERFERE,          // Interfere with an enemy's plans or operations
    TASK_ENFORCE_LAW,        // Enforce the law in a chaotic or lawless area
    TASK_RESTORE_HONOR,      // Restore the honor of a disgraced NPC or family
    TASK_PEACE_TREATY,       // Negotiate or enforce a peace treaty
    TASK_GATHER_HERBS,       // Gather specific herbs or plants
    TASK_CAMP,               // Set up or break down a camp
    TASK_SET_TRAPS,          // Set traps for animals or enemies
    TASK_CREATE_POTION,      // Create a potion using specific ingredients
    TASK_RECONSTITUTE,       // Reconstitute a broken or damaged artifact
    TASK_RENOVATE,           // Renovate or repair a building or structure
    TASK_EXTRACT_ORE,        // Extract ore from a mine or vein
    TASK_DOUSE_FLAMES,       // Douse flames in a burning building or area
    TASK_BLESS,              // Bless an object, location, or person
    TASK_PLUNDER,            // Plunder a village, ship, or enemy camp
    TASK_CONSECRATE,         // Consecrate a temple, altar, or sacred ground
    TASK_EVACUATE,           // Evacuate civilians or troops from a dangerous area
    TASK_DISGUISE,           // Disguise yourself or an NPC to infiltrate or avoid detection
    TASK_CONFRONT,           // Confront an NPC or creature about their actions
    TASK_WAGER,              // Wager on a game, race, or duel
    TASK_SAIL,               // Sail a ship to a specific destination
    TASK_MAINTAIN,           // Maintain equipment, a building, or machinery
    TASK_CLEANSE,            // Cleanse a cursed or polluted area
    TASK_HOLD_POSITION,      // Hold a position against enemies for a certain time
    TASK_MAP_TERRITORY,      // Map uncharted territory or a region
    TASK_INTERPRET,          // Interpret a prophecy, dream, or message
    TASK_TRANSLATE,          // Translate an ancient or foreign language
    TASK_ENFORCE_ORDER,      // Enforce order in a chaotic environment
    TASK_REPENT,             // Repent for a past mistake or sin
    TASK_INITIATE,           // Initiate a new member into a guild, order, or faction
    TASK_TEST_LOYALTY,       // Test the loyalty of an NPC or group
    TASK_SACRIFICE,          // Sacrifice an item, creature, or person for a greater cause
    TASK_COLLECT_TITHE,      // Collect a tithe or offering from NPCs
    TASK_PERFORM_RITUAL,     // Perform a ritual to summon, banish, or bless
    TASK_RESCUE_HOSTAGE,     // Rescue a hostage from enemy forces
    TASK_PROVOKE,            // Provoke an enemy or rival into action
    TASK_NEUTRALIZE,         // Neutralize a threat without killing
    TASK_SCOUR,              // Scour an area for resources, clues, or enemies
    TASK_MARK_LOCATION,      // Mark a location for future use or for others to find
    TASK_NAVIGATE,           // Navigate through dangerous or unknown territory
    TASK_MANAGE_RESOURCES,   // Manage resources such as food, water, or materials
    TASK_APPEASE,            // Appease an angry deity, spirit, or powerful NPC
    TASK_MOURN,              // Mourn the loss of a person, place, or thing
    TASK_ORGANIZE_EVENT,     // Organize an event such as a festival, gathering, or meeting
    TASK_ESCORT_PRISONER,    // Escort a prisoner to a specific location
    TASK_ENDURE,             // Endure a hardship or trial for a certain time
    TASK_BOND_WITH_CREATURE, // Bond with a creature or animal to gain its loyalty
    TASK_RECONCILE_FACTIONS, // Reconcile two or more factions that are at odds
    TASK_CONDUCT_RITUAL,     // Conduct a complex ritual with multiple steps
    TASK_OFFER_TRIBUTE,      // Offer tribute to a ruler, deity, or spirit
    TASK_EXILE,              // Exile an NPC or group from a community or region
    TASK_SUMMON_ALLY,        // Summon an ally or familiar to aid in a quest
    TASK_DEFUSE,             // Defuse a bomb or dangerous situation
    TASK_RELAY_MESSAGE,      // Relay a message between two parties
    TASK_FIND_RELIC,         // Find a lost or ancient relic
    TASK_DECIPHER_CODE,      // Decipher a code, puzzle, or riddle
    TASK_JUDGE_CONTEST,      // Judge a contest, competition, or duel
    TASK_ESTABLISH_BASE,     // Establish a base of operations in a new location
    TASK_DEFEAT_CHAMPION,    // Defeat a champion in battle or competition
    TASK_EMBARK_JOURNEY,     // Embark on a long and dangerous journey
    TASK_OVERTHROW,          // Overthrow a ruler or government
    TASK_PERFORM_CEREMONY,   // Perform a ceremonial duty or rite
    TASK_DEPLOY_TROOPS,      // Deploy troops to a strategic location
    TASK_CONDUCT_RESEARCH,   // Conduct research on a subject, item, or creature
    TASK_COUNTERFEIT,        // Create a counterfeit item, document, or currency
    TASK_OBTAIN_FAVOR,       // Obtain the favor of a powerful NPC or faction
    TASK_CHALLENGE_RIVAL,    // Challenge a rival to a duel, contest, or battle
    TASK_RECLAIM_LAND,       // Reclaim land from enemies or the wilderness
    TASK_PROTECT_ARTIFACT,   // Protect an artifact from theft or damage
    TASK_CREATE_ALLIANCE,    // Create an alliance between factions or NPCs
    TASK_TRAVERSE_DUNGEON,   // Traverse a dungeon or dangerous area
    TASK_DEFEND_STRONGHOLD,  // Defend a stronghold from an attack or siege
    TASK_UNDERGO_TRIAL,      // Undergo a trial of strength, wisdom, or courage
    TASK_REDEEM,             // Redeem an NPC or yourself from a past wrong
    TASK_RESTORE_BALANCE,    // Restore balance to a chaotic situation or ecosystem
    TASK_CONQUER_FORTRESS,   // Conquer a fortress or enemy stronghold
    TASK_DEPLOY_TRAP,        // Deploy a trap in a strategic location
    TASK_HARNESS_POWER,      // Harness a source of power or magic
    TASK_PLANT_EVIDENCE,     // Plant evidence to frame an NPC or mislead others
    TASK_RECON_VILLAGE,      // Recon a village or settlement for information
    TASK_ESCORT_VIP,         // Escort a VIP to a safe or strategic location
    TASK_CREATE_ART,         // Create a piece of art, music, or literature
    TASK_REPAIR_REPUTATION,  // Repair the reputation of an NPC, faction, or yourself
    TASK_FIND_HEIR,          // Find a missing heir to a throne or legacy
    TASK_REPAIR_BRIDGE,      // Repair a bridge to allow safe passage
    TASK_CONSTRUCT_BARRICADE,// Construct a barricade to defend against enemies
    TASK_INVOKE_DIVINE_HELP, // Invoke divine help or blessings in a dire situation
    TASK_PREPARE_FEAST,      // Prepare a feast or banquet for a special occasion
    TASK_OBSERVE,            // Observe an event or person without intervening
    TASK_MANAGE_SUPPLY_LINES,// Manage supply lines during a campaign or mission
    TASK_ARRANGE_MARRIAGE,   // Arrange a marriage for political or personal reasons
    TASK_FIND_CURE,          // Find a cure for a disease or curse
    TASK_RESCUE_ANIMAL,      // Rescue an endangered or trapped animal
    TASK_CONSTRUCT_MONUMENT, // Construct a monument to commemorate an event or person
    TASK_INVEST_FORTUNE,     // Invest a fortune in business, property, or trade
    TASK_HOST_GATHERING,     // Host a gathering or meeting of importance
    TASK_RESCUE_CHILD,       // Rescue a lost or kidnapped child
    TASK_SUBDUE_CREATURE,    // Subdue a rampaging creature without killing it
    TASK_MEDIATE,            // Mediate a dispute between NPCs or factions
    TASK_DESTROY_EVIDENCE,   // Destroy evidence of a crime or secret
    TASK_DEVELOP_TECH,       // Develop new technology, magic, or strategy
    TASK_NAVIGATE_POLITICS,  // Navigate complex political situations
    TASK_REVIVE,             // Revive an NPC from death or unconsciousness
    TASK_ESCORT_MERCHANT,    // Escort a merchant and their goods to a market or town
    TASK_INTERFERE_CEREMONY, // Interfere with a rival's ceremony or ritual
    TASK_DISMISS_ALLY,       // Dismiss an ally or companion from your service
    TASK_PERFORM_EXORCISM,   // Perform an exorcism to banish a spirit or demon
    TASK_SECURE_BORDER,      // Secure a border against invasion or smuggling
    TASK_RETRIEVE_TREASURE,  // Retrieve a treasure from a dangerous location
    TASK_HOST_CONTEST,       // Host a contest or tournament
    TASK_REUNITE_FAMILY,     // Reunite a family separated by conflict or tragedy
    TASK_SMELT_ORE,          // Smelt ore into usable metal
    TASK_ENCHANT_WEAPON,     // Enchant a weapon with special powers
    TASK_CREATE_GOLEM,       // Create or animate a golem or other construct
    TASK_REPLANT_FOREST,     // Replant a forest that has been destroyed
    TASK_REPLANT_FOREST,     // Replant a forest that has been destroyed
    TASK_RESURRECT,          // Resurrect an NPC or creature from the dead
    TASK_SILENCE,            // Silence an NPC or group to prevent them from speaking out
    TASK_SUBVERT,            // Subvert a rival or enemy's plans from within
    TASK_INVESTIGATE_CRIME,  // Investigate a crime scene or mystery
    TASK_DEFEND_CIVILIANS,   // Defend civilians from an attack or disaster
    TASK_TRAVEL_UNDERCOVER,  // Travel undercover to avoid detection
    TASK_EXTRACT_INFO,       // Extract information from a captured enemy or informant
    TASK_AWAKEN_ANCIENT,     // Awaken an ancient being or power
    TASK_CLEAR_DUNGEON,      // Clear a dungeon of enemies or traps
    TASK_CONDUCT_RAID,       // Conduct a raid on an enemy stronghold or village
    TASK_INTERCEPT_CARGO,    // Intercept a shipment of goods or weapons
    TASK_BUILD_FORTRESS,     // Build a fortress or stronghold
    TASK_ENFORCE_QUARANTINE, // Enforce a quarantine in a plague-ridden area
    TASK_MAINTAIN_MORALITY,  // Maintain the morality or faith of a group
    TASK_RECOVER_BODY,       // Recover a fallen ally's body from a dangerous location
    TASK_BRIBE_OFFICIAL,     // Bribe an official to gain favor or bypass a law
    TASK_PREVENT_WAR,        // Prevent a war between factions or nations
    TASK_INTERROGATE_PRISONER,// Interrogate a prisoner for vital information
    TASK_FORGE_ALLIANCE,     // Forge an alliance between warring factions
    TASK_OPERATE_VEHICLE,    // Operate a vehicle, such as a ship or airship
    TASK_HARNESS_ELEMENT,    // Harness the power of an elemental force
    TASK_BREED_ANIMALS,      // Breed animals for a specific purpose or trait
    TASK_QUELL_REBELLION,    // Quell a rebellion or uprising
    TASK_RECRUIT_SOLDIERS,   // Recruit soldiers for a military campaign
    TASK_SEEK_REDEMPTION,    // Seek redemption for past sins or failures
    TASK_ESCAPE_DUNGEON,     // Escape from a dungeon or prison
    TASK_TRACK_ENEMY,        // Track an enemy's movements and plans
    TASK_REPAIR_SHIP,        // Repair a damaged ship or vehicle
    TASK_MANAGE_ECONOMY,     // Manage the economy of a town or region
    TASK_CONSTRUCT_TEMPLE,   // Construct a temple or place of worship
    TASK_DEVELOP_CURE,       // Develop a cure for a deadly disease or poison
    TASK_EXCAVATE_RUINS,     // Excavate ruins to uncover lost artifacts or knowledge
    TASK_BARTER,             // Barter goods or services with NPCs or factions
    TASK_COMPOSE_LETTER,     // Compose an important letter or message
    TASK_NAVIGATE_SEAS,      // Navigate treacherous seas or waterways
    TASK_PROTECT_SECRET,     // Protect a secret from being discovered or revealed
    TASK_DISRUPT_RITUAL,     // Disrupt an enemy's ritual or ceremony
    TASK_NEGOTIATE_PEACE,    // Negotiate peace between warring factions
    TASK_REPAIR_RELIC,       // Repair a damaged or broken relic
    TASK_DISCOVER_ARTIFACT,  // Discover a lost or hidden artifact
    TASK_LEAD_EXPEDITION,    // Lead an expedition into unknown or dangerous territory
    TASK_TRAIN_COMPANION,    // Train a companion or pet in new skills
    TASK_CONVERT_ENEMY,      // Convert an enemy to your side or cause
    TASK_GATHER_REINFORCEMENTS, // Gather reinforcements to aid in a battle
    TASK_FREE_SLAVES,        // Free slaves or captives from bondage
    TASK_STOP_ASSASSINATION, // Stop an assassination attempt on an important NPC
    TASK_DISCOVER_LORE,      // Discover ancient lore or knowledge
    TASK_HARVEST_CROPS,      // Harvest crops from a field or farm
    TASK_EXTEND_TERRITORY,   // Extend the territory of a kingdom or faction
    TASK_REPEL_ATTACK,       // Repel an attack on a settlement or fortress
    TASK_PERFORM_SABOTAGE,   // Perform sabotage on enemy equipment or plans
    TASK_HIDE_EVIDENCE,      // Hide evidence of a crime or conspiracy
    TASK_CAPTURE_ANIMAL,     // Capture a wild or dangerous animal
    TASK_DEFUSE_CONFLICT,    // Defuse a conflict between NPCs or factions
    TASK_CONFRONT_FEARS,     // Confront and overcome personal fears or nightmares
    TASK_PROTECT_TRADER,     // Protect a trader on their journey through dangerous lands
    TASK_EXPAND_INFLUENCE,   // Expand the influence of your faction or cause
    TASK_RESTORE_ANCIENT_ORDER, // Restore an ancient order or organization
    TASK_INVESTIGATE_RUINS,  // Investigate ancient ruins for clues or treasure
    TASK_REINFORCE_WALLS,    // Reinforce the walls of a castle or city
    TASK_NAVIGATE_MOUNTAINS, // Navigate treacherous mountain paths
    TASK_PURIFY_SOURCE,      // Purify a corrupted or polluted source of water or power
    TASK_HEAL_WOUNDED,       // Heal the wounded after a battle or disaster
    TASK_ENCHANT_ARMOR,      // Enchant armor with magical properties
    TASK_AVENGE_DEATH,       // Avenge the death of a loved one or comrade
    TASK_LEARN_SPELL,        // Learn a new spell or magical ability
    TASK_FORM_COVENANT,      // Form a covenant with a powerful entity or faction
    TASK_TRAVERSE_DESERT,    // Traverse a dangerous desert region
    TASK_OVERCOME_CURSE,     // Overcome or lift a powerful curse
    TASK_BATTLE_ARENA,       // Battle in an arena for glory or rewards
    TASK_LOCATE_PORTAL,      // Locate a portal to another realm or dimension
    TASK_MANAGE_VILLAGE,     // Manage the affairs of a village or small town
    TASK_NEGOTIATE_TRADE,    // Negotiate trade agreements between factions
    TASK_REPEL_MONSTER,      // Repel a monster that threatens a town or settlement
    TASK_CONDUCT_RITUAL,     // Conduct a ritual to summon or banish a being
    TASK_EXPLORE_CAVES,      // Explore caves or underground tunnels
    TASK_CRAFT_WEAPON,       // Craft a weapon using special materials
    TASK_CLEAR_FOREST,       // Clear a forest of dangerous creatures or obstacles
    TASK_REPAIR_MACHINERY,   // Repair complex machinery or constructs
    TASK_RETRIEVE_SCROLL,    // Retrieve a lost or stolen scroll
    TASK_DEFEND_CARGO,       // Defend cargo during transport
    TASK_RECOVER_LOST_ITEM,  // Recover a lost or stolen item of importance
    TASK_NAVIGATE_SWAMP,     // Navigate through a treacherous swamp
    TASK_EXORCISE_HAUNTING,  // Exorcise a haunting from a location
    TASK_INVOKE_ANCIENT_RITES, // Invoke ancient rites or rituals for protection or power
    TASK_CONSTRUCT_BRIDGE,   // Construct a bridge to cross a river or chasm
    TASK_UNCOVER_HIDDEN_PATH,// Uncover a hidden path or secret passage
    TASK_FIND_LOST_CITY,     // Find a lost city or civilization
    TASK_BIND_DEMON,         // Bind a demon or spirit to prevent harm
    TASK_CREATE_ARMOR,       // Create armor for protection or battle
    TASK_MAP_CAVERNS,        // Map unexplored caverns or tunnels
    TASK_LIFT_SIEGE,         // Lift a siege on a city or fortress
    TASK_NAVIGATE_JUNGLE,    // Navigate through a dense jungle
    TASK_PURGE_CORRUPTION,   // Purge corruption from a person, place, or thing
    TASK_RETRIEVE_REMAINS,   // Retrieve the remains of a fallen ally or enemy
    TASK_CREATE_AMULET,      // Create a protective amulet or charm
    TASK_RESOLVE_DISPUTE,    // Resolve a dispute between NPCs or factions
    TASK_STOP_REBELLION,     // Stop a rebellion before it gains momentum
    TASK_REPEL_HORDE,        // Repel a horde of enemies attacking a settlement
    TASK_EXAMINE_ARTIFACT,   // Examine an artifact for clues or knowledge
    TASK_ARRANGE_TREATY,     // Arrange a treaty between warring factions
    TASK_PROTECT_REFUGEES,   // Protect refugees fleeing from war or disaster
    TASK_INTERCEPT_SPY,      // Intercept a spy before they deliver critical information
    TASK_DISCOVER_NEW_LAND,  // Discover new land for exploration or colonization
    TASK_RESCUE_SURVIVORS,   // Rescue survivors from a disaster or battle
    TASK_CULTIVATE_CROPS,    // Cultivate crops to ensure a harvest
    TASK_REPAIR_WEAPON,      // Repair a broken or damaged weapon
    TASK_REPAIR_WEAPON,      // Repair a broken or damaged weapon
    TASK_CONDUCT_INVESTIGATION, // Conduct an investigation into a mystery or crime
    TASK_DEFEND_VILLAGE,     // Defend a village from invaders or monsters
    TASK_INTERCEPT_COURIER,  // Intercept a courier carrying important messages or items
    TASK_PREPARE_DEFENSES,   // Prepare defenses for an impending attack
    TASK_SEEK_OUTLAW,        // Seek and capture or eliminate an outlaw
    TASK_CONVERT_FOLLOWERS,  // Convert followers to a new faith or cause
    TASK_SMUGGLE_GOODS,      // Smuggle goods past enemy lines or authorities
    TASK_PREPARE_RITUAL,     // Prepare a complex ritual requiring multiple steps
    TASK_REPAIR_ARMOR,       // Repair damaged armor
    TASK_CONDUCT_SURVEILLANCE, // Conduct surveillance on a target or location
    TASK_ACQUIRE_BLUEPRINTS, // Acquire blueprints for a device or building
    TASK_ESCORT_DIGNITARY,   // Escort a dignitary through dangerous territory
    TASK_RECRUIT_ALLIES,     // Recruit allies to support your cause
    TASK_CONSTRUCT_BARRACKS, // Construct barracks for housing soldiers
    TASK_REPAIR_FORTIFICATIONS, // Repair damaged fortifications or walls
    TASK_DEFUSE_AMBUSH,      // Defuse an ambush set by enemies
    TASK_GATHER_INTEL,       // Gather intelligence on enemy positions or plans
    TASK_CONDUCT_DIPLOMACY,  // Conduct diplomatic negotiations with a foreign power
    TASK_RETRIEVE_BLUEPRINTS,// Retrieve stolen or lost blueprints
    TASK_INTERCEPT_SMUGGLERS,// Intercept smugglers transporting illegal goods
    TASK_ESCAPE_PURSUIT,     // Escape from pursuers tracking you or your group
    TASK_REPAIR_GATE,        // Repair a damaged or broken gate
    TASK_CONSTRUCT_TOWER,    // Construct a tower for defense or observation
    TASK_RESUPPLY,           // Resupply a forward base or outpost with necessary goods
    TASK_CONDUCT_HEIST,      // Conduct a heist to steal valuable items or information
    TASK_NAVIGATE_MARSH,     // Navigate through a dangerous marsh or bog
    TASK_REPAIR_BRIDGE,      // Repair a bridge to restore a critical route
    TASK_DISRUPT_SMUGGLING,  // Disrupt smuggling operations in a region
    TASK_ENFORCE_LAW,        // Enforce the law in a lawless area or situation
    TASK_ESCORT_CONVOY,      // Escort a convoy through dangerous territory
    TASK_EXTINGUISH_FIRE,    // Extinguish a fire threatening a village or forest
    TASK_INVESTIGATE_MURDER, // Investigate a murder and find the culprit
    TASK_GATHER_MEDICINE,    // Gather medicine or herbs to treat the sick
    TASK_REPEL_RAIDERS,      // Repel raiders attacking a settlement
    TASK_DEFEND_CITADEL,     // Defend a citadel from an attacking force
    TASK_RECRUIT_MERCENARIES,// Recruit mercenaries for a specific mission or battle
    TASK_DISMANTLE_BOMB,     // Dismantle a bomb or explosive device
    TASK_RECONSTRUCT_EVENT,  // Reconstruct events of a crime or mystery
    TASK_CONDUCT_INTERVIEW,  // Conduct an interview to gather information or clues
    TASK_CLEAR_RUBBLE,       // Clear rubble from a road, building, or path
    TASK_DEFEND_TEMPLE,      // Defend a temple from desecration or attack
    TASK_CONSTRUCT_PALISADE, // Construct a palisade for temporary defense
    TASK_OBTAIN_PERMIT,      // Obtain a permit or authorization from authorities
    TASK_EXAMINE_CORPSE,     // Examine a corpse to determine cause of death
    TASK_PROTECT_MERCHANT,   // Protect a merchant from bandits or pirates
    TASK_FIND_LOST_MAP,      // Find a lost map leading to treasure or important locations
    TASK_RECLAIM_POSSESSION, // Reclaim a possession taken by enemies or thieves
    TASK_DISMANTLE_TRAP,     // Dismantle a trap set by enemies
    TASK_ACQUIRE_SEAL,       // Acquire a seal of authority or authenticity
    TASK_DEFEND_CARAVAN,     // Defend a caravan traveling through dangerous territory
    TASK_ESTABLISH_OUTPOST,  // Establish an outpost in a remote or strategic location
    TASK_RESOLVE_HOSTAGE_SITUATION, // Resolve a hostage situation peacefully or by force
    TASK_REPAIR_WELL,        // Repair a well to restore access to water
    TASK_SALVAGE_SHIPWRECK,  // Salvage a shipwreck for valuable goods or information
    TASK_AVERT_DISASTER,     // Avert a natural or magical disaster threatening a region
    TASK_EXORCISE_SPIRIT,    // Exorcise a malevolent spirit from a location or person
    TASK_SMUGGLE_PERSON,     // Smuggle a person out of a dangerous area
    TASK_NAVIGATE_WILDERNESS, // Navigate through a dense and perilous wilderness
    TASK_PROTECT_ARTISAN,    // Protect an artisan while they complete a crucial task
    TASK_REPAIR_ROAD,        // Repair a road to ensure safe travel
    TASK_COUNTER_INFILTRATION, // Counter enemy infiltration efforts within your ranks
    TASK_RESCUE_SAILORS,     // Rescue sailors stranded at sea or on a deserted island
    TASK_RECLAIM_TERRITORY,  // Reclaim territory lost to enemies or natural forces
    TASK_DISPEL_ILLUSION,    // Dispel an illusion hiding a truth or danger
    TASK_RESOLVE_FEUD,       // Resolve a feud between families, factions, or individuals
    TASK_RETRIEVE_KEY,       // Retrieve a key to unlock a door, chest, or magical barrier
    TASK_DEFEND_FARMSTEAD,   // Defend a farmstead from bandits or wild animals
    TASK_CONSTRUCT_AQUEDUCT, // Construct an aqueduct to bring water to a settlement
    TASK_FIND_LOST_CREW,     // Find and rescue a lost or missing crew from a ship or expedition
    TASK_EXCAVATE_TUNNEL,    // Excavate a tunnel for mining or escape purposes
    TASK_BOLSTER_MORALE,     // Bolster the morale of troops or a community facing hardship
    TASK_GUARD_PRISONER,     // Guard a high-profile prisoner until their trial or transport
    TASK_SECURE_RELIC,       // Secure a relic of great power or significance from enemies
    TASK_DEFEND_MARKET,      // Defend a marketplace from thieves or invaders
    TASK_GATHER_SUPPLIES,    // Gather supplies for an upcoming expedition or siege
    TASK_RECLAIM_WEAPONS,    // Reclaim weapons stolen by enemies or lost in battle
    TASK_CONDUCT_FORAGING,   // Conduct foraging in the wilderness for food and resources
    TASK_INVESTIGATE_OMEN,   // Investigate an ominous sign or prophecy
    TASK_RESCUE_DIPLOMAT,    // Rescue a diplomat captured by hostile forces
    TASK_PREPARE_SIEGE,      // Prepare for a siege by gathering resources and fortifying defenses
    TASK_CONSTRUCT_TREBUCHET, // Construct a trebuchet or other siege weapon
    TASK_CONTAIN_PLAGUE,     // Contain an outbreak of plague or disease in a community
    TASK_SUBDUE_REVOLT,      // Subdue a revolt among soldiers or civilians
    TASK_EXAMINE_DOCUMENTS,  // Examine documents for clues, forgery, or hidden information
    TASK_RESCUE_VILLAGERS,   // Rescue villagers taken captive by raiders or monsters
    TASK_INTERCEPT_REINFORCEMENTS, // Intercept enemy reinforcements before they can reach the battlefield
    TASK_DEFEND_LIBRARY,     // Defend a library or archive from destruction or theft
    TASK_REPAIR_WALL,        // Repair a breach in a wall or fortification
    TASK_GATHER_RESEARCH_MATERIALS, // Gather materials needed for important research
    TASK_CONSTRUCT_BATTERING_RAM, // Construct a battering ram to breach enemy gates
    TASK_NAVIGATE_CANYON,    // Navigate through a dangerous canyon or ravine
    TASK_DEFEND_TREASURY,    // Defend a treasury from robbers or enemy forces
    TASK_RESCUE_HOSTAGES,    // Rescue multiple hostages from a heavily guarded location
    TASK_RESTORE_HOPE,       // Restore hope to a demoralized community or group
    TASK_NEGOTIATE_RANSOM,   // Negotiate the ransom for a captured ally or valuable item
    TASK_REINFORCE_OUTPOST,  // Reinforce a vulnerable outpost with troops and supplies
    TASK_EXAMINE_ARTEFACT,    // Examine an artifact for magical properties or historical significance
    TASK_ESCAPE_TRAP,        // Escape from a trap set by enemies or natural forces
    TASK_CLEAR_INFESTATION,  // Clear an infestation of dangerous creatures from a location
    TASK_REPAIR_BRIDGE,      // Repair a damaged bridge to restore travel routes
    TASK_FORM_DEFENSIVE_PACT, // Form a defensive pact with another faction to deter aggression
    TASK_CONTAIN_MAGIC,      // Contain a runaway magical effect or creature
    TASK_CONSTRUCT_WATCHTOWER, // Construct a watchtower to oversee strategic areas
    TASK_SECURE_BORDERS,     // Secure the borders of a territory against incursions
    TASK_RECOVER_RELIC,      // Recover a stolen or lost relic of great importance
    TASK_CREATE_DIVERSION,   // Create a diversion to distract enemies or draw them away
    TASK_RESCUE_TRAPPED_MINERS, // Rescue miners trapped underground by a cave-in or collapse
    TASK_CONFRONT_WARLORD,   // Confront a warlord threatening regional stability
    TASK_NEGOTIATE_ALLIANCE, // Negotiate an alliance with a powerful faction or group
    TASK_DEFEND_WAYPOINT,    // Defend a strategic waypoint from enemy control
    TASK_RESCUE_SPY,         // Rescue a spy who has been captured behind enemy lines
    TASK_REPEL_SEIGE,        // Repel a siege on a fortress or city
    TASK_INTERCEPT_CARGO,    // Intercept valuable cargo being transported by enemies
    TASK_DEFEND_OUTPOST,     // Defend an outpost from being overrun by enemies
    TASK_ESCORT_PRISONERS,   // Escort prisoners safely to their destination
    TASK_SECURE_INTEL,       // Secure vital intelligence from enemy hands
    TASK_DEFEND_STRATEGIC_POINT, // Defend a strategic point critical to the overall battle plan
    TASK_RETRIEVE_ENCHANTED_ITEM, // Retrieve an enchanted item from a guarded location
    TASK_CONSTRUCT_TRENCH,   // Construct a trench for defensive purposes in battle
    TASK_PREPARE_AMBUSH,     // Prepare an ambush against approaching enemies
    TASK_DISCOVER_HIDDEN_TREASURE, // Discover hidden treasure in an unexplored area
    TASK_RESCUE_ROYALTY,     // Rescue a member of royalty from enemy capture
    TASK_DEFEND_CAPITAL,     // Defend the capital city from an invading force
    TASK_RECOVER_ANCIENT_TEXTS, // Recover ancient texts containing lost knowledge
    TASK_CONSTRUCT_CATAPULT, // Construct a catapult for siege warfare
    TASK_DEFEND_BRIDGE,      // Defend a bridge from enemy forces attempting to cross
    TASK_CLEAR_RIVER_BLOCKAGE, // Clear a blockage in a river to restore water flow
    TASK_REINFORCE_FORTRESS, // Reinforce a fortress under threat of attack
    TASK_CONDUCT_ESPIONAGE,  // Conduct espionage to gather enemy secrets or plans
    TASK_EXAMINE_RUINS,      // Examine ancient ruins for clues or artifacts
    TASK_CONSTRUCT_FORT,     // Construct a fort to secure a strategic location
    TASK_RECOVER_STOLEN_GOODS, // Recover stolen goods from bandits or pirates
    TASK_PREVENT_ASSASSINATION, // Prevent an assassination attempt on a key figure
    TASK_RESCUE_KIDNAPPED_CHILD, // Rescue a kidnapped child from their captors
    TASK_ESTABLISH_TRADE_ROUTE, // Establish a new trade route between distant regions
    TASK_DEFEND_SETTLEMENT,  // Defend a settlement from raiders or wild animals
    TASK_REPAIR_DAMAGED_DAM, // Repair a dam that is at risk of collapsing
    TASK_RESCUE_STRANDED_TRAVELERS, // Rescue stranded travelers from a remote location
    TASK_DEFEND_CASTLE,      // Defend a castle under siege by enemy forces
    TASK_RETRIEVE_MAGIC_SCROLL, // Retrieve a powerful magic scroll from a dangerous location
    TASK_CONSTRUCT_GATE,     // Construct a gate to protect a town or city
    TASK_REINFORCE_BRIDGE,   // Reinforce a bridge to withstand heavy use or attack
    TASK_EXAMINE_RELIC,      // Examine a relic for hidden powers or dangers
    TASK_CONSTRUCT_FENCE,    // Construct a fence to keep animals or enemies out
    TASK_NAVIGATE_STORMY_SEAS, // Navigate through stormy seas to reach a destination
    TASK_EXAMINE_MAGICAL_ARTIFACT, // Examine a magical artifact to determine its purpose or power
    TASK_CONSTRUCT_OUTPOST,  // Construct an outpost in a strategic or remote location
    TASK_RECOVER_SACRED_ITEM, // Recover a sacred item stolen from a temple or shrine
    TASK_ESCORT_WOUNDED,     // Escort wounded allies to a safe location for treatment
    TASK_DEFEND_FORT,        // Defend a fort from a concentrated enemy attack
    TASK_REPAIR_TOWN_WALL,   // Repair a damaged town wall to protect against invasion
    TASK_RESOLVE_POLITICAL_DISPUTE, // Resolve a political dispute before it escalates into conflict
    TASK_REPEL_INVADERS,     // Repel invaders from a territory or settlement
    TASK_CONSTRUCT_MOAT,     // Construct a moat around a fortress or castle
    TASK_ESCORT_SUPPLIES,    // Escort supplies to a besieged location
    TASK_DEFEND_BARRICADE,   // Defend a barricade from being breached by enemies
    TASK_RETRIEVE_LOST_TREASURE, // Retrieve lost treasure from a dangerous or hidden location
    TASK_NAVIGATE_BLIZZARD,  // Navigate through a blizzard to reach a destination safely
    TASK_REPEL_ENEMY_SCOUTS, // Repel enemy scouts from discovering strategic positions
    TASK_CONDUCT_RESCUE_OPERATION, // Conduct a rescue operation to save captured allies or civilians
    TASK_DEFEND_TOWER,       // Defend a tower from enemy assault
    TASK_CONSTRUCT_DEFENSIVE_POSITION, // Construct a defensive position to prepare for an imminent attack
    TASK_INTERCEPT_ENEMY_COMMANDER, // Intercept an enemy commander to disrupt their plans
    TASK_ESTABLISH_COMMUNICATION_LINE, // Establish a communication line between distant allies or outposts
    TASK_DEFEND_RIVER_CROSSING, // Defend a river crossing from enemy control
    TASK_RETRIEVE_ANCIENT_WEAPON, // Retrieve an ancient weapon of great power from a guarded location
    TASK_ESCORT_CIVILIANS,   // Escort civilians to safety during a crisis
    TASK_REPAIR_SHIP_HULL,   // Repair a ship's hull to prevent it from sinking
    TASK_DEFEND_FERRY,       // Defend a ferry from bandits or pirates during a crossing
    TASK_RESCUE_ABDUCTED_PRIEST, // Rescue an abducted priest from a hostile cult
    TASK_CONSTRUCT_RAMPARTS, // Construct ramparts to strengthen a defensive position
    TASK_CLEAR_CAVE_IN,      // Clear a cave-in to rescue trapped miners or adventurers
    TASK_DEFEND_ARCHIVE,     // Defend an archive containing valuable records or artifacts
    TASK_REINFORCE_WATCHTOWER, // Reinforce a watchtower to improve surveillance and defense
    TASK_REPEL_ENEMY_BOARDERS, // Repel enemy boarders attempting to seize control of a ship
    TASK_ESTABLISH_DEFENSIVE_RING, // Establish a defensive ring to protect a key location
    TASK_RECOVER_LOST_MANUSCRIPT, // Recover a lost manuscript of significant importance
    TASK_CONSTRUCT_BULWARK,  // Construct a bulwark to provide extra defense against attacks
    TASK_NAVIGATE_TREACHEROUS_TERRAIN, // Navigate treacherous terrain to reach an objective
    TASK_REPEL_ENEMY_PATROL, // Repel an enemy patrol from gaining information or territory
    TASK_CONDUCT_DEFENSIVE_DRILL, // Conduct a defensive drill to prepare for potential attacks
    TASK_REINFORCE_STRATEGIC_POSITION, // Reinforce a strategic position to prevent enemy advance
    TASK_RESCUE_MISSING_SCOUTS, // Rescue scouts who have gone missing behind enemy lines
    TASK_SECURE_WATER_SUPPLY, // Secure a water supply crucial for a settlement's survival
    TASK_DEFEND_BORDERS,     // Defend the borders of a kingdom or territory from incursions
    TASK_REPAIR_DOCKS,       // Repair docks to ensure ships can safely dock and unload supplies
    TASK_RECLAIM_ANCIENT_CITADEL, // Reclaim an ancient citadel taken over by enemies
    TASK_DEFEND_PORT,        // Defend a port city from enemy naval forces
    TASK_CONDUCT_SAFETY_INSPECTION, // Conduct a safety inspection of critical infrastructure
    TASK_REPAIR_DAMAGED_MONUMENT, // Repair a damaged monument of cultural or historical significance
    TASK_CONSTRUCT_BARRIERS, // Construct barriers to slow down or block enemy movement
    TASK_DEFEND_STRONGHOLD,  // Defend a stronghold from a major enemy offensive
    TASK_REPEL_NAVAL_INVASION, // Repel a naval invasion threatening coastal defenses
    TASK_ESTABLISH_TRADE_AGREEMENT, // Establish a trade agreement between distant kingdoms
    TASK_RESCUE_ABANDONED_CHILDREN, // Rescue abandoned children left in a dangerous area
    TASK_REINFORCE_CITY_WALLS, // Reinforce city walls to withstand a prolonged siege
    TASK_CONDUCT_COVERT_OP,  // Conduct a covert operation behind enemy lines
    TASK_DEFEND_HIGHLANDS,   // Defend the highlands from encroaching enemy forces
    TASK_RECOVER_LOST_ARTIFACT, // Recover a lost artifact with significant power
    TASK_CONSTRUCT_MOUNTAIN_PASS, // Construct a mountain pass to improve travel and trade routes
    TASK_RESCUE_ENDANGERED_ANIMALS, // Rescue endangered animals from poachers or habitat destruction
    TASK_DEFEND_AGAINST_RAID, // Defend a settlement against a raid from bandits or enemy troops
    TASK_RECLAIM_STOLEN_PROPERTY, // Reclaim stolen property taken by criminals or rival factions
    TASK_REPAIR_CITADEL_DEFENSES, // Repair the defenses of a citadel under threat
    TASK_CONDUCT_STRATEGIC_RETREAT, // Conduct a strategic retreat to regroup and fortify
    TASK_REPEL_FOREST_INVASION, // Repel an invasion of hostile forces in a forested area
    TASK_RESCUE_TRAPPED_SOLDIERS, // Rescue soldiers trapped behind enemy lines or in a collapsed structure
    TASK_DEFEND_HILLTOP,     // Defend a hilltop position from enemy forces trying to gain the high ground
    TASK_ESTABLISH_BORDER_OUTPOST, // Establish an outpost on the border to monitor enemy movements
    TASK_RETRIEVE_ABANDONED_SUPPLIES, // Retrieve abandoned supplies left in a dangerous location
    TASK_DEFEND_WINDMILL,    // Defend a windmill from being sabotaged or destroyed
    TASK_RESCUE_DOWNED_PILOT, // Rescue a downed pilot stranded in enemy territory
    TASK_REINFORCE_FORTIFIED_POSITION, // Reinforce a fortified position to prepare for a heavy attack
    TASK_CONSTRUCT_NAVAL_BASE, // Construct a naval base to strengthen maritime defenses
    TASK_SECURE_FOREST_EDGE, // Secure the edge of a forest to prevent enemy infiltration
    TASK_RECOVER_ROYAL_HEIRLOOM, // Recover a royal heirloom of immense value and significance
    TASK_DEFEND_MOUNTAIN_PASS, // Defend a mountain pass from being overtaken by enemy forces
    TASK_REPAIR_BARRACKS,    // Repair barracks to house soldiers or refugees
    TASK_ESTABLISH_SUPPLY_DEPOT, // Establish a supply depot to support ongoing operations
    TASK_REPEL_SEA_MONSTER,  // Repel a sea monster attacking ships or coastal settlements
    TASK_RESCUE_ABANDONED_ANIMALS, // Rescue abandoned animals left in a dangerous area
    TASK_DEFEND_OUTSKIRTS,   // Defend the outskirts of a town or city from enemy raids
    TASK_RECOVER_MAGIC_CRYSTAL, // Recover a magic crystal stolen by enemies
    TASK_CONDUCT_NIGHT_RAID, // Conduct a night raid on an enemy camp or supply line
    TASK_REPEL_BORDER_INCURSION, // Repel an enemy incursion across the border
    TASK_REPAIR_RIVER_DAM,   // Repair a river dam to prevent flooding
    TASK_SECURE_ANCIENT_TOMB, // Secure an ancient tomb from grave robbers or enemies
    TASK_RESCUE_CIVILIANS_FROM_CONFLICT_ZONE, // Rescue civilians caught in a conflict zone
    TASK_DEFEND_TRADE_ROUTE, // Defend a trade route from bandits or hostile forces
    TASK_RECOVER_ENEMY_PLANS, // Recover enemy plans to prevent them from executing their strategy
    TASK_CONSTRUCT_OBSERVATION_POST, // Construct an observation post to monitor enemy activity
    TASK_SECURE_SECRET_PASSAGE, // Secure a secret passage to ensure safe travel or escape
    TASK_REPEL_INVADING_ARMY, // Repel an invading army from your homeland
    TASK_REPAIR_FISHING_VILLAGE, // Repair a fishing village after a storm or raid
    TASK_RESCUE_PRISONERS_OF_WAR, // Rescue prisoners of war held by the enemy
    TASK_DEFEND_COASTLINE,   // Defend a coastline from enemy landings or raids
    TASK_CONDUCT_COUNTERINTELLIGENCE, // Conduct counterintelligence to prevent enemy espionage
    TASK_REINFORCE_SEIGE_DEFENSES, // Reinforce defenses during an ongoing siege
    TASK_CONSTRUCT_SUPPLY_LINE, // Construct a supply line to maintain troop morale and effectiveness
    TASK_RESOLVE_DISPUTE_OVER_TERRITORY, // Resolve a dispute between factions over territorial claims
    TASK_REPEL_NIGHT_ATTACK, // Repel a night attack by enemy forces
    TASK_REPAIR_TEMPLE_SHRINE, // Repair a temple or shrine damaged by conflict or disaster
    TASK_DEFEND_MOUNTAIN_FORTRESS, // Defend a mountain fortress from a major offensive
    TASK_CONDUCT_LONG_RANGE_RECONNAISSANCE, // Conduct long-range reconnaissance to gather critical intel
    TASK_RECLAIM_ABANDONED_FORT, // Reclaim an abandoned fort from enemies or wild creatures
    TASK_RESCUE_SHIPWRECKED_CREW, // Rescue a crew stranded after a shipwreck
    TASK_DEFEND_WAR_CAMP,    // Defend a war camp from enemy sabotage or attack
    TASK_CONSTRUCT_WATCHTOWER_NETWORK, // Construct a network of watchtowers for enhanced surveillance
    TASK_REPEL_SKIRMISHERS,  // Repel skirmishers attempting to harass your forces
    TASK_REPAIR_BATTLEFIELD_DEFENSES, // Repair battlefield defenses during or after an engagement
    TASK_RECOVER_LOST_SCOUTS, // Recover scouts who have been lost or captured during a mission
    TASK_DEFEND_HARBOR,      // Defend a harbor from a naval or amphibious assault
    TASK_ESTABLISH_SAFEHOUSE, // Establish a safehouse for intelligence or resistance operations
    TASK_REINFORCE_BATTALION, // Reinforce a battalion under heavy attack or low on morale
    TASK_REPEL_AMBUSH,       // Repel an ambush set by enemies during a march or expedition
    TASK_REPAIR_TREBUCHET,   // Repair a trebuchet damaged during siege warfare
    TASK_DEFEND_TRADING_POST, // Defend a trading post from bandits or hostile forces
    TASK_RESCUE_ARTISAN,     // Rescue an artisan kidnapped to force them into making weapons or art
    TASK_CONSTRUCT_BATTERY,  // Construct a battery of artillery or defensive weapons
    TASK_RECOVER_STOLEN_DOCUMENTS, // Recover documents stolen by enemy spies or thieves
    TASK_DEFEND_BARRACKS,    // Defend barracks housing soldiers or important personnel
    TASK_SECURE_ESCAPE_ROUTE, // Secure an escape route for retreating forces or refugees
    TASK_REPEL_DESERT_RAIDERS, // Repel raiders attacking from a desert region
    TASK_REPAIR_CATAPULT,    // Repair a catapult damaged in combat or through sabotage
    TASK_DEFEND_DESERT_OASIS, // Defend a desert oasis from hostile forces or nomads
    TASK_CONDUCT_TUNNEL_INSPECTION, // Conduct an inspection of tunnels to prevent collapses or enemy use
    TASK_RECOVER_WOUNDED_ALLIES, // Recover wounded allies from the battlefield
    TASK_CONSTRUCT_TRENCH_SYSTEM, // Construct a trench system for defense or assault preparation
    TASK_SECURE_MOUNTAIN_PEAK, // Secure a mountain peak for observation or tactical advantage
    TASK_REPEL_SWAMP_CREATURES, // Repel creatures attacking from a nearby swamp
    TASK_DEFEND_FOREST_GROVE, // Defend a forest grove sacred to local inhabitants or druidic orders
    TASK_RESCUE_HISTORIAN,   // Rescue a historian who has been captured or lost in a dangerous area
    TASK_CONSTRUCT_DEFENSIVE_FORTIFICATIONS, // Construct defensive fortifications to protect against invasions
    TASK_REPEL_WILDERNESS_ATTACK, // Repel an attack by hostile forces emerging from the wilderness
    TASK_REPAIR_RAMPARTS,    // Repair ramparts damaged during an ongoing siege or conflict
    TASK_SECURE_STRATEGIC_HILL, // Secure a hilltop offering a strategic advantage in battle
    TASK_DEFEND_BORDER_FORT, // Defend a fort on the border of enemy territory
    TASK_RESOLVE_CONFLICT_BETWEEN_CLANS, // Resolve a conflict between rival clans threatening regional stability
    TASK_REPEL_FOREST_RAIDERS, // Repel raiders attacking from the forest or jungle
    TASK_CONDUCT_DEEP_RECONNAISSANCE, // Conduct deep reconnaissance into enemy territory
    TASK_RECLAIM_OCCUPIED_TERRITORY, // Reclaim territory occupied by enemy forces or hostile creatures
    TASK_SECURE_RIVERBANK,   // Secure a riverbank to prevent enemy crossings or incursions
    TASK_DEFEND_SECRET_ENTRANCE, // Defend a secret entrance to a hidden base or stronghold
    TASK_RECOVER_FORGOTTEN_KNOWLEDGE, // Recover knowledge lost to time or hidden in ancient texts
    TASK_REPEL_MOUNTAIN_BANDITS, // Repel bandits attacking from the mountains
    TASK_RESCUE_MYSTIC,      // Rescue a mystic or seer captured by enemies or cultists
    TASK_CONSTRUCT_SPY_NETWORK, // Construct a network of spies to gather intelligence across a region
    TASK_DEFEND_ANCIENT_TEMPLE, // Defend an ancient temple from looters or invading forces
    TASK_REPAIR_ANCIENT_MONUMENT, // Repair an ancient monument of cultural or historical significance
    TASK_SECURE_FOREST_PASSAGE, // Secure a passage through a dense or enchanted forest
    TASK_RECOVER_STOLEN_TREASURE, // Recover treasure stolen by thieves or rival factions
    TASK_REPEL_JUNGLE_BEASTS, // Repel beasts attacking from a jungle or tropical region
    TASK_DEFEND_ANCIENT_LIBRARY, // Defend an ancient library from being ransacked or destroyed
    TASK_CONSTRUCT_FORTRESS_GATE, // Construct a gate to secure a fortress or walled city
    TASK_RESOLVE_CIVIL_UPRISING, // Resolve a civil uprising threatening stability in a city or region
    TASK_REPEL_MARAUDERS,    // Repel marauders attacking villages or supply lines
    TASK_CONDUCT_SEARCH_AND_DESTROY, // Conduct a search and destroy mission against a specific target
    TASK_RECOVER_LIFESAVING_MEDICINE, // Recover life-saving medicine from a dangerous or remote location
    TASK_DEFEND_SHIPPING_LANES, // Defend shipping lanes from pirates or enemy navies
    TASK_RESCUE_CAPTURED_LEADER, // Rescue a captured leader before they are executed or turned against you
    TASK_SECURE_FORTIFIED_CAMP, // Secure a camp heavily fortified but under threat of attack
    TASK_REPEL_ENEMY_RAID,   // Repel an enemy raid on your base or territory
    TASK_REPAIR_BREACHED_WALL, // Repair a wall breached by enemy forces during an assault
    TASK_DEFEND_WATER_SUPPLY, // Defend a water supply from being poisoned or cut off
    TASK_RECOVER_DANGEROUS_ARTIFACT, // Recover an artifact that could cause great harm if left in the wrong hands
    TASK_CONDUCT_RECON_MISSION, // Conduct a reconnaissance mission to gather vital information
    TASK_RECLAIM_ABANDONED_MINE, // Reclaim an abandoned mine overrun by enemies or dangerous creatures
    TASK_REPEL_WINTER_OFFENSIVE, // Repel a winter offensive launched by the enemy
    TASK_DEFEND_TRADING_HUB, // Defend a trading hub crucial for economic stability
    TASK_SECURE_SECRET_DOCUMENTS, // Secure documents that contain secret or sensitive information
    TASK_RECOVER_STOLEN_HORSES, // Recover horses stolen by bandits or enemy forces
    TASK_CONSTRUCT_WATCHTOWER_AT_BORDER, // Construct a watchtower at the border for surveillance and defense
    TASK_REPEL_NAVAL_RAIDERS, // Repel raiders attacking from the sea or river
    TASK_DEFEND_BORDER_WATCHTOWER, // Defend a border watchtower from being overrun by enemy forces
    TASK_RESCUE_CAPTIVE_CHILDREN, // Rescue children taken captive by enemies or slavers
    TASK_RECOVER_STOLEN_IDOLS, // Recover idols stolen from a temple or sacred site
    TASK_DEFEND_MINING_OPERATION, // Defend a mining operation from hostile forces or sabotage
    TASK_CONSTRUCT_SIEGE_ENGINE, // Construct a siege engine for an upcoming battle
    TASK_REPEL_ENEMY_CHAMPION, // Repel an enemy champion challenging your forces or territory
    TASK_SECURE_SMUGGLER_ROUTE, // Secure a smuggler's route to prevent illegal trade or enemy infiltration
    TASK_DEFEND_TOWN_HALL,   // Defend the town hall from an insurrection or attack
    TASK_RESCUE_ABANDONED_WAGON, // Rescue a wagon left behind in enemy territory or dangerous conditions
    TASK_RECOVER_MAGIC_AMULET, // Recover a magic amulet of significant power or importance
    TASK_REPEL_BARBARIAN_RAID, // Repel a raid by barbarian tribes attacking a settlement or outpost
    TASK_DEFEND_SUPPLY_CONVOY, // Defend a supply convoy transporting crucial goods
    TASK_RECLAIM_HOLY_SITE,  // Reclaim a holy site taken over by enemies or desecrated by invaders
    TASK_SECURE_TRADE_AGREEMENT, // Secure a trade agreement to strengthen economic ties between regions
    TASK_REPEL_ENEMY_VANGUARD, // Repel an enemy vanguard seeking to establish a foothold
    TASK_DEFEND_FARM,        // Defend a farm from being raided or destroyed
    TASK_RECOVER_LAWLESS_TERRITORY, // Recover a territory overrun by outlaws or bandits
    TASK_REPAIR_FORTRESS_WALL, // Repair a damaged fortress wall to prevent further breaches
    TASK_CONSTRUCT_CITY_GATE, // Construct a gate to secure a city or major settlement
    TASK_REPEL_ENEMY_INVADERS, // Repel enemy invaders from your territory or homeland
    TASK_DEFEND_MOUNTAIN_OUTPOST, // Defend a mountain outpost from being overtaken by enemies
    TASK_RECLAIM_ANCIENT_WEAPON, // Reclaim an ancient weapon lost or hidden in enemy territory
    TASK_SECURE_VALLEY_PASSAGE, // Secure a valley passage critical for movement or trade
    TASK_REPEL_SWAMP_RAIDERS, // Repel raiders attacking from a swampy region
    TASK_CONSTRUCT_HILL_FORT, // Construct a fort on a hill for strategic defense
    TASK_REPAIR_MILITARY_BASE, // Repair a military base damaged by enemy action
    TASK_DEFEND_ANCIENT_FORT, // Defend an ancient fort from modern threats
    TASK_RECOVER_ENCHANTED_TALISMAN, // Recover an enchanted talisman from a dangerous location
    TASK_REPEL_NOMADIC_RAID, // Repel a raid by nomadic tribes
    TASK_SECURE_OLD_CASTLE, // Secure an old castle to use as a stronghold
    TASK_CONDUCT_EVACUATION, // Conduct an evacuation of civilians from a dangerous area
    TASK_DEFEND_RURAL_VILLAGE, // Defend a rural village from raiders or invaders
    TASK_RETRIEVE_HIDDEN_TREASURE, // Retrieve treasure hidden in a well-guarded location
    TASK_CONSTRUCT_BUNKERS, // Construct bunkers for troops or civilians
    TASK_REPAIR_BATTLEFIELD_EQUIPMENT, // Repair equipment used in recent battles
    TASK_DEFEND_TEMPLE, // Defend a temple from desecration or attack
    TASK_REPEL_INVADING_HORDES, // Repel hordes of enemies attacking from multiple directions
    TASK_SECURE_SUPPLY_ROUTE, // Secure a route used for transporting supplies
    TASK_RECOVER_ANTIQUITIES, // Recover valuable antiquities from thieves or looters
    TASK_DEFEND_TREASURE_VAULT, // Defend a vault containing valuable treasure
    TASK_CONSTRUCT_FIELD_DEFENSES, // Construct defenses in an open field to resist an assault
    TASK_REPAIR_OLD_WATCHTOWER, // Repair an old watchtower for improved surveillance
    TASK_REPEL_SIEGE_WEAPONS, // Repel enemy siege weapons targeting your defenses
    TASK_SECURE_ENCHANTED_FOREST, // Secure a forest with magical properties from intruders
    TASK_DEFEND_REMOTE_OUTPOST, // Defend a remote outpost from encroaching enemies
    TASK_REPAIR_MAGE_TOWER, // Repair a tower used by mages for their magical operations
    TASK_RECOVER_VITAL_SUPPLIES, // Recover essential supplies lost or stolen in conflict
    TASK_CONSTRUCT_DEFENSIVE_GATES, // Construct gates to enhance the defense of a fortification
    TASK_REPEL_CLAN_WARLORD, // Repel a warlord leading a clan assault
    TASK_DEFEND_HISTORICAL_SITE, // Defend a site of historical significance from destruction
    TASK_REPAIR_FORTIFIED_WALLS, // Repair walls of a fortification to restore its defensive capability
    TASK_SECURE_TACTICAL_POSITION, // Secure a position of tactical importance in an ongoing conflict
    TASK_RECOVER_STOLEN_EQUIPMENT, // Recover equipment stolen by enemies or thieves
    TASK_DEFEND_CIVILIAN_CONVOY, // Defend a convoy of civilians traveling through dangerous areas
    TASK_CONSTRUCT_REINFORCED_BARRICADES, // Construct barricades reinforced for extra protection
    TASK_REPEL_WILD_CREATURES, // Repel wild creatures threatening settlements or supply lines
    TASK_DEFEND_IMPORTANT_BRIDGE, // Defend a bridge critical for transportation and defense
    TASK_RECOVER_ENEMY_WEAPONS, // Recover weapons lost or captured from the enemy
    TASK_SECURE_HIDDEN_LAIR, // Secure a hidden lair from intruders or hostile forces
    TASK_REPAIR_DAMAGED_PALACE, // Repair a palace damaged by conflict or disaster
    TASK_DEFEND_CULTURAL_CENTER, // Defend a cultural center from attack or vandalism
    TASK_CONSTRUCT_SHIELD_WALL, // Construct a shield wall to protect against a siege or assault
    TASK_REPEL_TROLLS, // Repel trolls attacking from mountainous or rocky terrain
    TASK_RECOVER_ANCIENT_SCROLLS, // Recover ancient scrolls from a protected location
    TASK_DEFEND_FOREST_VILLAGE, // Defend a village situated within a dense forest from invaders
    TASK_REPAIR_MYSTIC_OBELISK, // Repair a mystic obelisk of significance to local magical traditions
    TASK_CONSTRUCT_FORTRESS_TOWER, // Construct a tower within a fortress for better defense and observation
    TASK_SECURE_ABANDONED_OUTPOST, // Secure an outpost that was previously abandoned
    TASK_DEFEND_NATIONAL_MONUMENT, // Defend a monument of national importance from damage or theft
    TASK_RECOVER_HEIRLOOM_WEAPON, // Recover a weapon of significant familial or historical value
    TASK_CONDUCT_STRATEGIC_SURVEY, // Conduct a survey to assess strategic locations and conditions
    TASK_REPEL_DARK_MAGIC_USERS, // Repel dark magic users attempting to infiltrate or attack
    TASK_SECURE_HIDDEN_TREASURE, // Secure treasure hidden in a secret location
    TASK_DEFEND_RITUAL_SITE, // Defend a site used for important rituals from disruption or attack
    TASK_REPAIR_MAGICAL_BARRIER, // Repair a magical barrier protecting a critical location
    TASK_CONSTRUCT_ARTILLERY_EMPLACEMENTS, // Construct emplacements for artillery to enhance defensive capabilities
    TASK_RECOVER_ANCESTRAL_TREASURE, // Recover treasure passed down through generations from a safe location
    TASK_DEFEND_MYSTICAL_SPRING, // Defend a mystical spring with significant magical properties
    TASK_REPAIR_AIRSHIP, // Repair an airship damaged during aerial conflict or travel
    TASK_SECURE_AIRSPACE, // Secure airspace from aerial threats or intrusions
    TASK_CONDUCT_MASS_EVACUATION, // Conduct an evacuation of a large population from a threatened area
    TASK_DEFEND_VITAL_INFRASTRUCTURE, // Defend critical infrastructure from sabotage or attack
    TASK_RECOVER_DAMAGED_ARTIFACTS, // Recover artifacts damaged during conflicts or raids
    TASK_CONSTRUCT_REPAIR_STATIONS, // Construct stations for repairing equipment or fortifications
    TASK_REPEL_CULT_INVADERS, // Repel invaders from a cult or religious faction
    TASK_SECURE_ANCIENT_TREASURE_TROVE, // Secure a trove of ancient treasures from being plundered
    TASK_DEFEND_ELITE_TROOPS, // Defend a unit of elite troops from enemy assault or capture
    TASK_REPAIR_FORTRESS_CANNONS, // Repair cannons within a fortress to restore their effectiveness
    TASK_CONSTRUCT_REINFORCED_BUNKERS, // Construct bunkers with additional reinforcement for strategic protection
    TASK_RECOVER_ILLUMINATED_MANUSCRIPTS, // Recover manuscripts with illuminated texts of great value
    TASK_DEFEND_SECRET_BASE, // Defend a base hidden from enemy eyes from being discovered or attacked
    TASK_REPAIR_DEFENSIVE_TURRETS, // Repair turrets used for defending against enemy attacks
    TASK_SECURE_HIGHLY_ENCHANTED_ITEM, // Secure an item of significant magical enchantment
    TASK_CONDUCT_ELITE_STRIKE_MISSION, // Conduct a high-risk strike mission with elite operatives
    TASK_DEFEND_AGAINST_MAGIC_ATTACKS, // Defend against magical attacks from enemy spellcasters
    TASK_RECOVER_HISTORICAL_RELICS, // Recover relics of historical importance from a secure location
    TASK_REPAIR_ANCIENT_DEFENSES, // Repair ancient defenses to restore their original protective capability
    TASK_CONSTRUCT_MAGICALLY_ENCHANTED_BARRIERS, // Construct barriers with magical enchantments for enhanced defense
    TASK_DEFEND_CULTURAL_HERITAGE, // Defend a site of cultural heritage from being attacked or damaged
    TASK_SECURE_VITAL_MAGIC_ARTIFACT, // Secure a magic artifact crucial to your forces or plans
    TASK_REPAIR_DILAPIDATED_KEEP, // Repair a keep that has fallen into disrepair and disuse
    TASK_DEFEND_CRUCIAL_RESOURCE, // Defend a resource essential for survival or strategic advantage
    TASK_RECOVER_FAMOUS_BATTLE_STANDARD, // Recover a battle standard of historic significance
    TASK_CONSTRUCT_TACTICAL_FORTIFICATIONS, // Construct fortifications designed for tactical advantage in battle
    TASK_SECURE_ENCHANTED_WEAPONS, // Secure weapons imbued with enchantments from falling into enemy hands
    TASK_REPEL_ARMORED_RAIDERS, // Repel raiders equipped with heavy armor and advanced weaponry
    TASK_DEFEND_RITUAL_ALTAR, // Defend an altar used for significant rituals from desecration
    TASK_RECOVER_MAGICAL_ENCHANTMENTS, // Recover enchantments lost or stolen from magical artifacts
    TASK_REPAIR_UNDERGROUND_BUNKERS, // Repair bunkers located underground to ensure their functionality
    TASK_CONSTRUCT_MAGICAL_OBELISKS, // Construct obelisks with magical properties for enhanced defense or power
    TASK_DEFEND_VITAL_WAREHOUSE, // Defend a warehouse holding essential supplies or equipment
    TASK_RECOVER_ANCESTRAL_ARTIFACTS, // Recover artifacts with ancestral or cultural importance
    TASK_REPAIR_ENCHANTED_FORGE, // Repair a forge used for creating enchanted items
    TASK_SECURE_COVERT_OPERATION_BASE, // Secure a base used for covert operations and intelligence gathering
    TASK_DEFEND_COVERT_MEETING_PLACE, // Defend a location used for secret meetings from discovery or attack
    TASK_RECOVER_ANCIENT_TEXTS, // Recover ancient texts of great knowledge
    TASK_CONSTRUCT_ELITE_DEFENSIVE_LINE, // Construct a defensive line designed to withstand elite enemy forces
    TASK_DEFEND_VITAL_TRANSPORT_HUB, // Defend a transport hub crucial for logistics and troop movement
    TASK_SECURE_MYSTICAL_ARTIFACTS, // Secure artifacts with mystical properties from being stolen or destroyed
    TASK_RECOVER_COVERT_DOCUMENTS, // Recover documents related to secret operations or intelligence
    TASK_REPAIR_OBSOLETE_TURRETS, // Repair outdated turrets to bring them back into operational use
    TASK_DEFEND_ENCHANTED_LIBRARY, // Defend a library containing powerful or magical knowledge from attack
    TASK_CONSTRUCT_RITUAL_DEFENSES, // Construct defenses specifically designed to protect sacred or ritual sites
    TASK_REPEL_INVASION_FROM_THE_SEA, // Repel an invasion force approaching by sea
    TASK_SECURE_ANCIENT_TEMPLE_GATES, // Secure the gates of an ancient temple to prevent unauthorized access
    TASK_RECOVER_SCARCE_RESOURCES, // Recover resources that are rare or critical for your operations
    TASK_DEFEND_LANDMARK_STRUCTURE, // Defend a landmark structure of great significance from damage or occupation
    TASK_CONSTRUCT_FORTIFIED_PERIMETER, // Construct a fortified perimeter around a key installation or settlement
    TASK_REPAIR_CRUCIAL_COMMUNICATIONS, // Repair communication systems vital for coordinating defense or operations
    TASK_SECURE_ENCHANTED_BARRIERS, // Secure barriers enhanced with magical protections from being breached
    TASK_DEFEND_AIRBORNE_OPERATIONS, // Defend operations involving airborne units or assets from enemy interference
    TASK_RECOVER_HISTORICAL_STATUES, // Recover statues of historical importance from potential damage or theft
    TASK_REPAIR_MAGICAL_CONDUITS, // Repair conduits used to channel magical energy or spells
    TASK_CONSTRUCT_REINFORCED_TRENCHE, // Construct a trench with additional reinforcements for better protection
    TASK_SECURE_MYSTICAL_FOREST, // Secure a forest imbued with mystical properties from intruders
    TASK_DEFEND_KEY_TRADING_POST, // Defend a trading post crucial for commerce and supply routes
    TASK_RECOVER_ANCIENT_SCROLLS, // Recover ancient scrolls containing important historical or magical information
    TASK_CONDUCT_HIGH_STAKES_NEGOTIATIONS, // Conduct negotiations of great importance to resolve conflicts or secure alliances
    TASK_DEFEND_AGAINST_NECROMANCY, // Defend against necromantic forces or magic targeting your forces or territory
    TASK_REPAIR_EARTHEN_WALLS, // Repair earthen walls that have been damaged or breached
    TASK_SECURE_HIDDEN_MINE, // Secure a hidden mine containing valuable resources or artifacts
    TASK_RECOVER_WOUNDED_VETERANS, // Recover and provide aid to wounded veterans from recent conflicts
    TASK_DEFEND_ANCIENT_SHRINE, // Defend an ancient shrine from desecration or attack
    TASK_CONSTRUCT_MAGICAL_WARDS, // Construct wards to protect against magical attacks or intrusions
    TASK_REPAIR_HISTORICAL_BUILDINGS, // Repair buildings of historical importance damaged in recent events
    TASK_SECURE_FORTIFIED_OUTPOST, // Secure an outpost with fortifications to prevent enemy capture
    TASK_DEFEND_VITAL_CROSSROADS, // Defend a crossroads critical for movement and supply routes
    TASK_RECOVER_DAMAGED_RITUAL_ARTIFACTS, // Recover artifacts used in rituals that have been damaged or lost
    TASK_REPAIR_ENCHANTED_GATES, // Repair gates enhanced with magical protections to restore their functionality
    TASK_CONSTRUCT_STRATEGIC_OBSERVATION_POSTS, // Construct posts for strategic observation of enemy movements and positions
    TASK_SECURE_VITAL_WEAPONSTOCKS, // Secure stocks of vital weapons from being seized or destroyed
    TASK_DEFEND_CRUCIAL_FORTIFICATION, // Defend a fortification crucial for maintaining control over a region
    TASK_RECOVER_ANCIENT_WEAPONS, // Recover weapons of ancient origin with significant power or historical value
    TASK_REPAIR_CIVILIAN_INFRASTRUCTURE, // Repair infrastructure essential for civilian life and support
    TASK_SECURE_CRITICAL_LANDMARKS, // Secure landmarks of great strategic or symbolic importance from threat
    TASK_DEFEND_AGAINST_SPY_INTRUSION, // Defend against espionage attempts targeting sensitive areas or information
    TASK_RECOVER_SENSITIVE_DATA, // Recover data critical to operations or security from compromised sources
    TASK_CONSTRUCT_ENCHANTED_CAVERN_DEFENSES, // Construct defenses within a cavern with magical enchantments
    TASK_REPAIR_TREASURE_VAULTS, // Repair vaults used to store valuable treasures and artifacts
    TASK_SECURE_GREAT_LIBRARY, // Secure a large library holding extensive and valuable knowledge
    TASK_DEFEND_INFLUENTIAL_LEADER, // Defend a leader of great influence from threats or attempts on their life
    TASK_RECOVER_CRITICAL_MEDICAL_SUPPLIES, // Recover medical supplies essential for treatment and recovery
    TASK_REPAIR_DAMAGED_BATTLEFIELD_STRATEGY, // Repair and adjust strategies used on the battlefield based on recent damage
    TASK_CONSTRUCT_ELITE_PROTECTIVE_ARMOR, // Construct armor designed for elite troops or high-value targets
    TASK_SECURE_ANCIENT_SECRET_TREASURE, // Secure treasure hidden in ancient locations of historical importance
    TASK_DEFEND_HIDDEN_MILITARY_BASE, // Defend a secret military base from discovery or attack
    TASK_RECOVER_DAMAGED_MAGIC_ITEMS, // Recover magic items damaged during recent conflicts or raids
    TASK_REPAIR_BATTLEFIELD_ARTIFACTS, // Repair artifacts found on the battlefield essential for historical or strategic reasons
    TASK_CONSTRUCT_PROTECTIVE_RUNES, // Construct runes for protection against magical or physical threats
    TASK_SECURE_LEGENDARY_SITES, // Secure sites of legendary significance from threats or exploitation
    TASK_DEFEND_STRATEGIC_TRADE_ROUTE, // Defend a trade route vital for commerce and resource distribution
    TASK_RECOVER_MYSTICAL_ARTIFACTS, // Recover artifacts with mystical significance from dangerous locations
    TASK_REPAIR_CRITICAL_INDUSTRIAL_EQUIPMENT, // Repair equipment crucial for industrial operations and production
    TASK_CONSTRUCT_RITUALISTIC_DEFENSES, // Construct defenses based on ritualistic or magical designs for enhanced protection
    TASK_SECURE_HIDDEN_MYSTICAL_ARTIFACTS, // Secure mystical artifacts hidden from general knowledge or access
    TASK_DEFEND_CULTURAL_INSTITUTIONS, // Defend institutions dedicated to culture and the arts from attack or damage
    TASK_RECOVER_DAMAGED_STATUES, // Recover and restore statues of artistic or historical significance
    TASK_REPAIR_DEMOLISHED_FORTIFICATIONS, // Repair fortifications that have been demolished or heavily damaged
    TASK_CONSTRUCT_UNDERGROUND_BUNKERS, // Construct bunkers underground to provide protection and concealment
    TASK_SECURE_HISTORICAL_SITES, // Secure sites of historical importance from damage, theft, or unauthorized access
    TASK_DEFEND_MYSTICAL_CAVES, // Defend caves with mystical properties from intrusion or desecration
    TASK_RECOVER_EXPERT_CRAFTSMAN, // Recover a skilled craftsman who can create or repair valuable items
    TASK_REPAIR_ANCIENT_DEFENSIVE_STRUCTURES, // Repair ancient structures designed for defensive purposes
    TASK_CONSTRUCT_ENCHANTED_PROTECTIVE_BARRIERS, // Construct barriers enhanced with enchantments for superior protection
    TASK_SECURE_ECONOMICALLY_VITAL_RESOURCES, // Secure resources critical for economic stability and growth
    TASK_DEFEND_AGAINST_MAGICAL_BEASTS, // Defend against beasts with magical properties or abilities
    TASK_RECOVER_ENCHANTED_SCROLLS, // Recover scrolls imbued with magic from secure locations
    TASK_REPAIR_HIGH_PRIORITY_EQUIPMENT, // Repair equipment deemed high priority for ongoing operations
    TASK_CONSTRUCT_FORTIFIED_RESEARCH_LAB, // Construct a research lab with fortified defenses for scientific or magical work
    TASK_SECURE_MAGICALLY_ENHANCED_TREASURE, // Secure treasure with magical enhancements from theft or damage
    TASK_DEFEND_TACTICAL_OUTPOSTS, // Defend outposts critical for tactical operations and defense
    TASK_RECOVER_RARE_ALCHEMICAL_REAGENTS, // Recover reagents used in alchemy that are rare or essential
    TASK_REPAIR_PROTECTIVE_MAGIC_CONDUITS, // Repair conduits used to channel protective magic or enchantments
    TASK_CONSTRUCT_ADVANCED_DEFENSIVE_FORTIFICATIONS, // Construct advanced fortifications for improved defense against formidable threats
    TASK_DEFEND_ENCHANTED_SPRING, // Defend a spring with magical properties from exploitation or attack
    TASK_RECOVER_LOST_CIVILIZATION_ARTIFACTS, // Recover artifacts from a lost civilization of great historical or magical significance
    TASK_REPAIR_ADVANCED_SEA_DEFENSES, // Repair advanced defenses designed to protect against sea-based attacks
    TASK_CONSTRUCT_SECURE_RITUAL_CHAMBERS, // Construct chambers designed for performing high-security rituals
    TASK_SECURE_CRITICAL_MAGE_TOWER, // Secure a mage tower vital for magical operations and research
    TASK_DEFEND_HEAVILY_FORTIFIED_CITY, // Defend a city with extensive fortifications from sieges or attacks
    TASK_RECOVER_ANCIENT_TECHNOLOGY, // Recover technology of ancient origin with significant power or utility
    TASK_REPAIR_EXPERIMENTAL_WEAPONS, // Repair weapons of experimental design used for cutting-edge combat
    TASK_CONSTRUCT_PROTECTIVE_ENCHANTMENTS, // Construct enchantments to protect key locations or items from magical harm
    TASK_SECURE_ALCHEMICAL_REACTORS, // Secure reactors used in alchemy or magical experiments from threats
    TASK_DEFEND_SUPPLY_DEPOT, // Defend a depot holding critical supplies from sabotage or theft
    TASK_RECOVER_DAMAGED_ANTIQUITIES, // Recover and restore antiquities damaged in recent conflicts
    TASK_REPAIR_TACTICAL_ENGINES, // Repair engines used in tactical operations or war machines
    TASK_CONSTRUCT_RESEARCH_STATIONS, // Construct stations dedicated to research in strategic or magical fields
    TASK_SECURE_MAGICAL_TRANSMISSION_TOWERS, // Secure towers used for transmitting magical energy or signals from interference
    TASK_DEFEND_CONSTRUCTED_EMBASSIES, // Defend embassies constructed in dangerous regions from attack or breach
    TASK_RECOVER_OBSOLETE_ARTIFACTS, // Recover artifacts that have fallen out of use but are still of historical value
    TASK_REPAIR_SECURE_CHESTS, // Repair chests used to store valuable or sensitive items
    TASK_CONSTRUCT_FORTIFIED_COMMAND_BUNKERS, // Construct command bunkers with advanced fortifications for strategic command
    TASK_SECURE_ANCIENT_POWER_SOURCES, // Secure sources of ancient power from theft or misuse
    TASK_DEFEND_INFLUENTIAL_ORGANIZATIONS, // Defend organizations of great influence or power from hostile actions
    TASK_RECOVER_LOST_TREASURES, // Recover treasures that have been lost or hidden over time
    TASK_REPAIR_EXOTIC_ARTIFACTS, // Repair artifacts of exotic origin that are crucial for various operations
    TASK_CONSTRUCT_SUPPLY_CHAIN_DEFENSES, // Construct defenses to protect critical supply chains from disruption
    TASK_SECURE_LEGENDARY_TREASURE_CACHES, // Secure caches of treasure with legendary status from plunder
    TASK_DEFEND_HIGH_PRIORITY_RESEARCH_LABS, // Defend research labs with high-priority projects from attack
    TASK_RECOVER_SENSITIVE_EQUIPMENT, // Recover equipment essential for operations that has been compromised or lost
    TASK_REPAIR_ANCIENT_MILITARY_EQUIPMENT, // Repair equipment of ancient military origin for restoration and use
    TASK_CONSTRUCT_ENCHANTED_SECURITY_SYSTEMS, // Construct security systems with enchantments for heightened protection
    TASK_SECURE_HISTORICAL_RESEARCH_ARCHIVES, // Secure archives containing historical research from unauthorized access or damage
    TASK_DEFEND_CRITICAL_WAR_SUPPLIES, // Defend supplies critical for ongoing war efforts from attack or theft
    TASK_RECOVER_MAGICALLY_ENHANCED_GEAR, // Recover gear enhanced with magical properties that has been lost or stolen
    TASK_REPAIR_FORTIFIED_MILITARY_OUTPOSTS, // Repair military outposts with fortifications to restore their effectiveness
    TASK_CONSTRUCT_STRATEGIC_MAGICAL_OBSERVATORIES, // Construct observatories designed for monitoring magical events or phenomena
    TASK_SECURE_ANCIENT_ENCHANTED_RELICS, // Secure relics with ancient enchantments from being disturbed or misused
    TASK_DEFEND_HIGHLY_SENSITIVE_SITES, // Defend sites with highly sensitive information or assets from breaches
    TASK_RECOVER_VITAL_MAGICAL_ESSENCES, // Recover essences crucial for magical operations or artifacts
    TASK_REPAIR_ADVANCED_ENCHANTMENT_DEVICES, // Repair devices used for advanced enchantments or magical operations
    TASK_CONSTRUCT_ENCHANTED_TACTICAL_GATES, // Construct gates with enchantments for tactical advantage in defense
    TASK_SECURE_CRITICAL_ENCHANTED_RESOURCES, // Secure resources imbued with enchantments essential for various uses
    TASK_DEFEND_AGAINST_MAGICAL_INTERFERENCE, // Defend against interference from magical sources or entities
    TASK_RECOVER_OLD_TRADITIONAL_WEAPONS, // Recover traditional weapons with significant historical or cultural importance
    TASK_REPAIR_HIGH_ECH_ARTIFACTS, // Repair artifacts with high technological advancements from damage or malfunction
    TASK_CONSTRUCT_MAGICALLY_PROTECTED_BARRIERS, // Construct barriers protected with magic for superior defense
    TASK_SECURE_ENCHANTED_TREASURE_VAULTS, // Secure vaults holding treasure with magical enhancements
    TASK_DEFEND_KEY_SACRED_SITES, // Defend sacred sites of great importance from desecration or attack
    TASK_RECOVER_HISTORIC_MILITARY_DOCUMENTS, // Recover documents of historic military importance from secure locations
    TASK_REPAIR_ADVANCED_PROTECTIVE_SYSTEMS, // Repair systems designed for advanced protection against various threats
    TASK_CONSTRUCT_DEFENSIVE_RITUALS, // Construct defensive measures based on rituals for enhanced protection
    TASK_SECURE_HIDDEN_MAGIC_TREASURES, // Secure hidden treasures with magical properties from discovery or theft
    TASK_DEFEND_CRITICAL_MAGICAL_GATES, // Defend gates with significant magical importance from breach or damage
    TASK_RECOVER_ANCIENT_ENCHANTMENTS, // Recover enchantments from ancient sources that have been lost or hidden
    TASK_REPAIR_SECURE_RESEARCH_EQUIPMENT, // Repair equipment used in secure research environments
    TASK_CONSTRUCT_TACTICAL_ENCHANTED_FORTIFICATIONS, // Construct fortifications with tactical enchantments for strategic defense
    TASK_SECURE_VITAL_ANCIENT_GRIMOIRES, // Secure grimoires containing ancient knowledge from unauthorized access
    TASK_DEFEND_INTERNATIONAL_TREATY_SITES, // Defend sites related to international treaties from violations or attacks
    TASK_RECOVER_HISTORIC_CONSTRUCTION_PLANS, // Recover plans related to historic constructions of strategic importance
    TASK_REPAIR_DEFENSIVE_ENCHANTMENTS, // Repair enchantments used to protect key defensive structures or assets
    TASK_CONSTRUCT_ADVANCED_MAGE_BARRIERS, // Construct barriers with advanced magical properties for high-level protection
    TASK_SECURE_ENCHANTED_WEAPON_STORAGE, // Secure storage areas for weapons with magical enhancements
    TASK_DEFEND_AGAINST_RITUALISTIC_ATTACKS, // Defend against attacks based on ritualistic or magical practices
    TASK_RECOVER_HISTORICAL_ARTIFACTS, // Recover artifacts with significant historical value from secure locations
    TASK_REPAIR_TACTICAL_MAGIC_BARRIERS, // Repair barriers designed for tactical use in magical defenses
    TASK_CONSTRUCT_SUPPLY_CHAIN_SECURITY, // Construct security measures for protecting critical supply chains
    TASK_SECURE_ENCHANTED_MILITARY_INSTALLATIONS, // Secure military installations with magical enhancements
    TASK_DEFEND_HIGH_VALUE_RESEARCH_FACILITIES, // Defend research facilities of high value from threats and breaches
    TASK_RECOVER_DAMAGED_MYSTICAL_ARTIFACTS, // Recover and restore artifacts of mystical importance that have been damaged
    TASK_REPAIR_ADVANCED_DEFENSIVE_CONDUITS, // Repair conduits used for advanced defensive measures
    TASK_CONSTRUCT_MAGICAL_HARVESTING_STATIONS, // Construct stations for harvesting magical resources or energy
    TASK_SECURE_CRUCIAL_MAGICAL_TRANSPORTS, // Secure transport routes or vessels used for moving magical resources
    TASK_DEFEND_RESEARCH_AND_DEVELOPMENT_SITES, // Defend sites dedicated to research and development from attacks or sabotage
    TASK_RECOVER_EXOTIC_RITUAL_ARTIFACTS, // Recover artifacts used in exotic rituals from danger or theft
    TASK_REPAIR_ADVANCED_STRATEGIC_ARTIFACTS, // Repair strategic artifacts with advanced features or technology
    TASK_CONSTRUCT_FORTIFIED_MAGIC_WARDS, // Construct wards with fortifications to enhance magical protection
    TASK_SECURE_LEGENDARY_ENCHANTED_ITEMS, // Secure items of legendary enchantment from theft or destruction
    TASK_DEFEND_ANTIQUITY_STORAGE_FACILITIES, // Defend facilities storing valuable antiquities from breach or damage
    TASK_RECOVER_HISTORICAL_RESEARCH_EQUIPMENT, // Recover equipment used in historical research from loss or damage
    TASK_REPAIR_TACTICAL_ENCHANTED_ARTIFACTS, // Repair artifacts with tactical enchantments for improved functionality
    TASK_CONSTRUCT_ENCHANTED_PROTECTIVE_SHELTERS, // Construct shelters with magical protection for safety and security
    TASK_SECURE_ANCIENT_MAGIC_SECRETS, // Secure secrets related to ancient magic from being exposed or misused
    TASK_DEFEND_CRITICAL_MAGIC_RESEARCH, // Defend research focused on magic from disruption or attack
    TASK_RECOVER_DAMAGED_HISTORICAL_RELICS, // Recover and restore relics of historical significance that have been damaged
};

#endif