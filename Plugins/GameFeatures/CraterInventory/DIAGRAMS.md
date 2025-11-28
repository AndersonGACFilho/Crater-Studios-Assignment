# CraterInventory - Visual Diagram Reference

> Quick reference guide for all Mermaid diagrams used in the CraterInventory documentation.

---

## 📊 Available Diagrams

This document provides an overview of all visual diagrams available in the CraterInventory documentation. All diagrams use [Mermaid](https://mermaid.js.org/) syntax for automatic rendering.

---

## System Architecture Diagrams

### 1. Three-Layer Architecture
**Location**: README.MD - Architecture Section  
**Type**: Layered Architecture Diagram

```mermaid
graph TB
    subgraph PresentationLayer["PRESENTATION LAYER"]
        InventoryUI["Inventory UI<br/>(Elementus)"]
        EquipmentUI["Equipment UI<br/>(Elementus)"]
        ItemTooltip["Item Tooltip<br/>(Elementus)"]
    end
    
    subgraph BusinessLogicLayer["BUSINESS LOGIC LAYER"]
        CraterManager["UCraterInventoryManagerComponent<br/>(Server Authority, Equipment, GAS Integration)"]
    end
    
    subgraph DataAccessLayer["DATA ACCESS LAYER"]
        ElementusStorage["Elementus Storage<br/>(Replication)"]
        LyraFragments["Lyra Item Fragments<br/>(Gameplay Logic)"]
    end
    
    PresentationLayer <--> BusinessLogicLayer
    BusinessLogicLayer <--> DataAccessLayer
    ElementusStorage <--> LyraFragments
    
    style PresentationLayer fill:#e1f5ff
    style BusinessLogicLayer fill:#fff4e1
    style DataAccessLayer fill:#f0f0f0
```

**Purpose**: Shows the separation of concerns between UI, business logic, and data storage.

---

### 2. Component Hierarchy
**Location**: ARCHITECTURE.MD - Component Hierarchy Section  
**Type**: Component Relationship Diagram

```mermaid
graph TB
    CraterMgr["UCraterInventoryManagerComponent<br/>(Business Logic)"]
    ElementusComp["UElementusInventoryComponent<br/>(Storage)"]
    ItemsArray["TArray&lt;FElementusItemInfo&gt;<br/>(Replicated)"]
    EquippedItems["TArray&lt;FCraterEquippedItemEntry&gt;<br/>(Equipment State)"]
    GASHandles["GAS Handles:<br/>• Ability Handles<br/>• Effect Handles<br/>• Tag Container"]
    ASC["UAbilitySystemComponent<br/>(Cached reference)"]
    GrantedResources["Abilities, Effects, Tags<br/>granted/revoked"]
    
    CraterMgr --> ElementusComp
    ElementusComp --> ItemsArray
    CraterMgr --> EquippedItems
    EquippedItems --> GASHandles
    CraterMgr -.->|Cached| ASC
    ASC --> GrantedResources
    
    style CraterMgr fill:#FF9800
    style ElementusComp fill:#2196F3
    style ASC fill:#F44336
    style EquippedItems fill:#9C27B0
```

**Purpose**: Illustrates how components are structured and their dependencies.

---

## Integration Diagrams

### 3. Lyra Framework Integration
**Location**: README.MD - Integration Section  
**Type**: Flow Diagram

```mermaid
flowchart TD
    Experience["Lyra Experience"]
    GFAction["GameFeatureAction_AddComponents"]
    Actor["PlayerState/Pawn"]
    CraterMgr["UCraterInventoryManagerComponent"]
    ElementusMgr["UElementusInventoryComponent"]
    LyraItemDef["ULyraInventoryItemDefinition"]
    Fragment["UCraterInventoryFragment_Equippable"]
    ASC["UAbilitySystemComponent"]
    
    Experience -->|"Activates"| GFAction
    GFAction -->|"Injects at runtime"| Actor
    Actor -->|"Receives component"| CraterMgr
    Actor -->|"Receives component"| ElementusMgr
    CraterMgr <-->|"Bridges to"| ElementusMgr
    CraterMgr -->|"Uses"| LyraItemDef
    LyraItemDef -->|"Contains"| Fragment
    Fragment -->|"Grants abilities/effects to"| ASC
    
    style Experience fill:#4CAF50
    style GFAction fill:#2196F3
    style CraterMgr fill:#FF9800
    style Fragment fill:#9C27B0
    style ASC fill:#F44336
```

**Purpose**: Shows how CraterInventory integrates into Lyra's Game Feature system.

---

### 4. Bridge Pattern (Data Integration)
**Location**: ARCHITECTURE.MD - Data Bridge Section  
**Type**: Class Diagram

```mermaid
classDiagram
    class UElementusItemData {
        <<Abstraction>>
        +ItemId
        +DisplayName
        +MaxStackSize
    }
    
    class UElementusItemData_LyraIntegration {
        +TSoftClassPtr~LyraItemDefinition~
        +bCanBeEquipped
        +LoadLyraDefinitionSync()
        +FindLyraFragment()
    }
    
    class ULyraInventoryItemDefinition {
        <<Implementation>>
        +Fragments
        +DisplayName
        +FindFragmentByClass()
    }
    
    UElementusItemData <|-- UElementusItemData_LyraIntegration
    UElementusItemData_LyraIntegration ..> ULyraInventoryItemDefinition : Bridge
    
    note for UElementusItemData_LyraIntegration "Soft reference prevents<br/>hard dependency"
```

**Purpose**: Demonstrates the Bridge pattern used to decouple Elementus from Lyra.

---

## Network Flow Diagrams

### 5. Server-Client Authority Model
**Location**: README.MD - Network Architecture Section  
**Type**: Sequence Diagram

```mermaid
sequenceDiagram
    participant Client
    participant Server
    
    Client->>Server: Server_AddItem(ItemId, Qty)
    Note over Server: Validate Request
    Note over Server: Execute Transaction
    Note over Server: Update Storage
    Server->>Client: Replicate Storage (OwnerOnly)
    Note over Client: OnInventoryUpdate (Delegate)
    Note over Client: UI Refresh
```

**Purpose**: Shows the client-server interaction for inventory operations.

---

### 6. Item Addition Flow (Detailed)
**Location**: ARCHITECTURE.MD - Data Flow Section  
**Type**: Sequence Diagram

```mermaid
sequenceDiagram
    participant ClientUI as Client UI
    participant CraterMgr as CraterInventoryManager
    participant Server
    participant ElementusComp as ElementusInventoryComponent
    participant ClientComp as Client (Replicated)
    participant UI as UI Widget
    
    ClientUI->>CraterMgr: [UI Button Click]
    CraterMgr->>Server: Server_AddItem(ItemId, Qty) [RPC]
    
    Note over Server: Server_AddItem_Implementation
    Server->>Server: ValidateStorageReady()
    Server->>Server: Log transaction
    Server->>ElementusComp: AddItems()
    Note over ElementusComp: Find existing stack or create new
    Note over ElementusComp: Update TArray<FElementusItemInfo>
    Note over ElementusComp: Mark for replication
    
    ElementusComp->>ClientComp: [Replication - OwnerOnly]
    
    Note over ClientComp: OnRep_Items()
    ClientComp->>ClientComp: Broadcast OnInventoryUpdate
    ClientComp->>CraterMgr: HandleStorageUpdate()
    Note over CraterMgr: ValidateEquippedItems()
    Note over CraterMgr: SyncLyraInstances()
    CraterMgr->>UI: Broadcast OnInventoryChanged
    UI->>UI: RefreshInventoryDisplay()
```

**Purpose**: Complete flow from UI click to storage update and UI refresh.

---

### 7. Item Pickup Flow (Simple)
**Location**: README.MD - Network Architecture Section  
**Type**: Sequence Diagram

```mermaid
sequenceDiagram
    participant Client
    participant Server
    
    Client->>Server: 1. Interact with world item
    Server->>Server: 2. Validate interaction
    Server->>Server: 3. Server_AddItem
    Server->>Server: 4. Storage update
    Server->>Client: 5. Receive replication
    Note over Client: 6. UI update
```

**Purpose**: Simplified view of picking up items from the world.

---

## Equipment & GAS Integration Diagrams

### 8. Equipment Flow with GAS
**Location**: ARCHITECTURE.MD - Equipment Flow Section  
**Type**: Sequence Diagram (Complex)

```mermaid
sequenceDiagram
    participant Client
    participant CraterMgr as CraterInventoryManager
    participant Server
    participant ASC as AbilitySystemComponent
    participant ClientASC as Client ASC
    participant UI
    
    Note over Client: [Drag Item to Slot]
    Client->>Client: CanEquipItem(SlotIdx, EquipSlot)
    Note over Client: ValidateStorageReady()<br/>ValidateInventorySlot()<br/>Check IsItemEquipped()<br/>Check bCanBeEquipped<br/>Check tag requirements
    
    Client->>Server: [If Success] Server_EquipItem(SlotIdx, EquipSlot) [RPC]
    
    Note over Server: Server_EquipItem_Implementation
    Server->>Server: CanEquipItem() [Re-validate on server]
    Server->>Server: Auto-select slot if needed<br/>FindAppropriateEquipmentSlot()
    Server->>Server: Unequip existing item in target slot<br/>Server_UnequipItem_Impl()<br/>RevokeAbilitiesFromItem()
    
    Note over Server: Update EquippedItems[TargetSlot]
    Server->>Server: Set InventorySlotIndex<br/>Copy ItemInfo<br/>CreateLyraInstanceFromItem()
    Note over Server: Load LyraItemDefinition<br/>Create ULyraInventoryItemInstance<br/>Initialize fragments
    
    Note over Server: GrantAbilitiesFromItem()
    Server->>ASC: GetOwnerASC()
    Server->>ASC: GetEquippableFragment()
    
    loop For each AbilityClass
        ASC->>ASC: GiveAbility()
        ASC->>Server: Store FGameplayAbilitySpecHandle
    end
    
    loop For each EffectClass
        ASC->>ASC: ApplyGameplayEffectSpecToSelf()
        ASC->>Server: Store FActiveGameplayEffectHandle
    end
    
    Server->>ASC: AddLooseGameplayTags()
    Server->>Server: Store FGameplayTagContainer
    
    Server->>Server: Mark EquippedItems for replication
    Server->>Server: Broadcast OnItemEquipped
    
    Server->>Client: [Replication - OwnerOnly]
    Client->>Client: OnRep_EquippedItems()
    Client->>UI: Broadcast OnInventoryChanged
    UI->>UI: UpdateEquipmentSlots()
    
    ASC->>ClientASC: [ASC replicates abilities/effects/tags]
    Note over ClientASC: Abilities now usable on client
```

**Purpose**: Complete equipment flow including GAS ability granting.

---

### 9. Equipment Flow (Simple)
**Location**: README.MD - Network Architecture Section  
**Type**: Sequence Diagram

```mermaid
sequenceDiagram
    participant Client
    participant Server
    participant ASC as AbilitySystemComponent
    
    Note over Client: 1. Click "Equip" button
    Client->>Client: 2. CanEquipItem (local validation)
    Client->>Server: 3. Server_EquipItem (RPC)
    Server->>Server: 4. CanEquipItem (authoritative validation)
    Server->>Server: 5. Update equipment array
    Server->>ASC: 6. Grant abilities to ASC
    Server->>Client: 7. Receive equipment replication
    ASC->>Client: 8. Receive ability replication (from ASC)
    Note over Client: 9. UI update
    Note over Client: 10. Ability available for use
```

**Purpose**: Simplified equipment flow for quick understanding.

---

## Lifecycle Diagrams

### 10. Game Feature Lifecycle
**Location**: ARCHITECTURE.MD - GameFeatureAction Section  
**Type**: State Diagram

```mermaid
stateDiagram-v2
    [*] --> Loaded: Game Feature Loaded
    Loaded --> Activating: OnGameFeatureActivating()
    Activating --> RegisterComponents: Get GameFrameworkComponentManager
    RegisterComponents --> WaitingForActors: For each ComponentEntry<br/>AddComponentRequest()
    WaitingForActors --> ComponentAdded: Target Actor Spawns
    ComponentAdded --> Active: Component Manager adds components
    Active --> Deactivating: Game Feature Deactivating
    Deactivating --> Cleanup: Component Manager removes components
    Cleanup --> [*]: Clean up request handles
```

**Purpose**: Shows the complete lifecycle of the Game Feature plugin.

---

## Backend Architecture Diagrams

### 11. Backend-Authoritative System
**Location**: README.MD - Future Enhancements Section  
**Type**: Layered Architecture Diagram

```mermaid
graph TB
    subgraph Backend["Backend Services"]
        ItemSvc["Item Service"]
        LootSvc["Loot Service"]
        MatchSvc["Match Service"]
    end
    
    subgraph GameServer["Game Server (UE)"]
        ServerInv["UServerInventoryService<br/>(Spawned on DS only)"]
        Note1["• Fetches player stash from backend<br/>• Validates item operations<br/>• Generates loot from loot tables<br/>• Commits extracted items post-match"]
    end
    
    subgraph ClientLayer["Client"]
        ClientNote["• Displays items (read-only authority)<br/>• Sends requests (equip, use, drop)<br/>• Receives state updates"]
    end
    
    Backend <-->|REST/gRPC| GameServer
    GameServer <-->|Replication| ClientLayer
    
    style Backend fill:#4CAF50
    style GameServer fill:#2196F3
    style ClientLayer fill:#FF9800
```

**Purpose**: Proposed architecture for backend-authoritative inventory in extraction shooter.

---

## Blueprint/Usage Diagrams

### 12. Blueprint: Get Inventory Component
**Location**: QUICKSTART.MD - Blueprint Examples Section  
**Type**: Flow Diagram

```mermaid
graph LR
    A[Get Owning Player Pawn] --> B[Find Crater Inventory Manager]
    B --> C[Return: CraterInventoryManagerComponent]
    
    style B fill:#2196F3
    style C fill:#4CAF50
```

**Purpose**: Blueprint node flow for finding inventory component.

---

### 13. Blueprint: Add Item on Pickup
**Location**: QUICKSTART.MD - Blueprint Examples Section  
**Type**: Flow Diagram

```mermaid
graph TD
    A[Event: OnInteract] --> B[Get Crater Inventory Manager]
    B --> C[Server Add Item]
    C --> D[Destroy Pickup Actor]
    
    C -.->|Parameters| E[Item ID: from pickup actor<br/>Quantity: 1]
    
    style A fill:#4CAF50
    style C fill:#FF9800
    style D fill:#F44336
```

**Purpose**: Blueprint implementation for world item pickup.

---

### 14. Blueprint: Equip Item from UI
**Location**: QUICKSTART.MD - Blueprint Examples Section  
**Type**: Flow Diagram

```mermaid
graph TD
    A[Event: OnSlotClicked] --> B[Get Crater Inventory Manager]
    B --> C[Can Equip Item]
    C --> D{Result}
    D -->|Success| E[Server Equip Item]
    D -->|Failed| F[Show Error Message]
    
    C -.->|Parameters| G[Slot Index: from event<br/>Equipment Slot: -1 auto]
    F -.->|Use| H[Enum to string for feedback]
    
    style A fill:#4CAF50
    style E fill:#2196F3
    style F fill:#F44336
```

**Purpose**: Blueprint implementation for UI-driven item equipping.

---

### 15. Blueprint: Healing Ability Flow
**Location**: QUICKSTART.MD - Create Healing Ability Section  
**Type**: Flow Diagram

```mermaid
graph TD
    A[Event: ActivateAbility] --> B[Get Ability System Component]
    B --> C[Make Outgoing Gameplay Effect Spec]
    C --> D[Apply Gameplay Effect Spec To Self]
    D --> E[End Ability]
    
    C -.->|Parameters| F[Gameplay Effect: GE_Heal<br/>Level: 1.0]
    
    style A fill:#4CAF50
    style E fill:#F44336
```

**Purpose**: Blueprint implementation for consumable healing item.

---

### 16. Simplified Architecture (Index)
**Location**: DOCUMENTATION_INDEX.MD - Key Concepts Section  
**Type**: Layered Diagram

```mermaid
graph TB
    subgraph UILayer["UI Layer"]
        Widgets["Elementus Widgets"]
    end
    
    subgraph BusinessLogic["Business Logic"]
        Manager["CraterInventoryManager"]
    end
    
    subgraph DataLayer["Data Layer"]
        Storage["Elementus Storage"]
        Fragments["Lyra Fragments"]
    end
    
    UILayer <--> BusinessLogic
    BusinessLogic <--> DataLayer
    
    style UILayer fill:#e1f5ff
    style BusinessLogic fill:#fff4e1
    style DataLayer fill:#f0f0f0
```

**Purpose**: Quick overview of system architecture for navigation guide.

---

## Diagram Color Legend

### Standard Colors Used

- 🟢 **Green (#4CAF50)**: Entry points, events, backend services
- 🔵 **Blue (#2196F3)**: Core components, storage, game server
- 🟠 **Orange (#FF9800)**: Business logic, managers, client
- 🟣 **Purple (#9C27B0)**: Equipment/specialized components
- 🔴 **Red (#F44336)**: Exit points, ASC, destructive actions
- 🔵 **Light Blue (#e1f5ff)**: UI/Presentation layers
- 🟡 **Light Yellow (#fff4e1)**: Business logic layers
- ⚪ **Gray (#f0f0f0)**: Data access layers

---

## Viewing the Diagrams

### GitHub
Diagrams render automatically when viewing `.md` files on GitHub.

### VS Code
Install the "Markdown Preview Mermaid Support" extension:
```
ext install bierner.markdown-mermaid
```

### JetBrains IDEs (Rider, IntelliJ)
Install the "Mermaid" plugin from the marketplace.

### Online Viewers
- [Mermaid Live Editor](https://mermaid.live/)
- [GitHub Gist](https://gist.github.com/) (supports Mermaid)

### Exporting
Use the Mermaid Live Editor to export diagrams as:
- PNG images
- SVG vectors
- PDF documents

---

## Diagram Quick Reference Table

| # | Diagram Name | Type | Location | Purpose |
|---|--------------|------|----------|---------|
| 1 | Three-Layer Architecture | Layered | README.MD | System structure |
| 2 | Component Hierarchy | Component | ARCHITECTURE.MD | Component relationships |
| 3 | Lyra Integration | Flow | README.MD | Integration flow |
| 4 | Bridge Pattern | Class | ARCHITECTURE.MD | Design pattern |
| 5 | Authority Model | Sequence | README.MD | Network model |
| 6 | Item Addition Flow | Sequence | ARCHITECTURE.MD | Detailed data flow |
| 7 | Item Pickup Flow | Sequence | README.MD | Simple pickup |
| 8 | Equipment Flow (Detailed) | Sequence | ARCHITECTURE.MD | Complete equip flow |
| 9 | Equipment Flow (Simple) | Sequence | README.MD | Simple equip flow |
| 10 | Game Feature Lifecycle | State | ARCHITECTURE.MD | Plugin lifecycle |
| 11 | Backend Architecture | Layered | README.MD | Future design |
| 12 | BP: Get Component | Flow | QUICKSTART.MD | Blueprint usage |
| 13 | BP: Add Item | Flow | QUICKSTART.MD | Blueprint usage |
| 14 | BP: Equip Item | Flow | QUICKSTART.MD | Blueprint usage |
| 15 | BP: Healing Ability | Flow | QUICKSTART.MD | Blueprint usage |
| 16 | Simplified Arch | Layered | DOCUMENTATION_INDEX.MD | Quick overview |

---

**Total Diagrams**: 16  
**Diagram Types**: Sequence (7), Flow (6), Layered (3), State (1), Class (1), Component (1)

---

**Last Updated**: 2025-11-28  
**Mermaid Version**: Compatible with Mermaid 8.0+

