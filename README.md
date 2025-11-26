# Crater Studios – Engineering Homework Assignment

**Position:** Systems & Gameplay Engineer  
**Project:** Multiplayer Extraction Shooter (Lyra-based)

## Goal
Demonstrate your ability to understand and extend the **Lyra framework**, with a focus on gameplay systems like inventory, itemization, and ability integration.

## Assignment Overview
Using a **free inventory interface system** from the Unreal Engine Fab Marketplace (such as Grid Inventory) for item visualization and equipping/unequipping and dropping items as a player (your choice) - and integrating it into the Lyra current inventory system in a way that:

* Shows the player’s current inventory using your or the plugin UMGs.
* Reflects equipped items and their gameplay impact (e.g., ability granting, stat bonuses).
* Use the Lyra built-in inventory systems for logic and backend work and integrate it with that plugin visualization and functionality.
* Uses Lyra’s modular design properly and cleanly.

The goal is to see you:
1.  Understand Lyra’s architecture.
2.  Make good decisions about modularity and authority.
3.  Show some ability to extend and build gameplay systems.

## What You Should Learn in the Process

### 1. Lyra Core Architecture
Understand the flow of how Lyra initializes:
* GameInstance, GameMode, GameState, PlayerState, PlayerController, Pawn Component.
* Explore the Experience system and how it determines what gets loaded per game mode or player type.

### 2. Itemization Framework
Investigate Lyra's:
* `InventoryManagerComponent`
* `EquipmentManagerComponent`
* `ItemDefinition`, `ItemInstance`, and `ItemFragment`
* Learn how to display and bind inventory/equipment to UMG using a modular approach.

### 3. Gameplay Ability System (GAS)
* Dynamically grant or remove Abilities based on equipped items.
* Use Gameplay Effects to apply stat bonuses (e.g., healing, movement speed).
* Integrate input-bound abilities depending on equipment.

## Deliverables
A project or plugin module added to Lyra showing:
* A UMG widget showing the player's inventory & equipped items.
* Abilities/effects granted dynamically based on what’s equipped.
* Some visualizations of animations and meshes on the Lyra shooter core mannequin (Lyra's main character).

A short doc explaining:
* What inventory system you picked.
* What you integrated and why.
* What you learned or struggled with.

## Bonus Tasks (Optional but Valued)

### Grenade Belt
* Equip a grenade item → Press **G** to throw it (uses a gameplay ability).

### Healing Belt
* Equip a healing item → Press **G** to heal yourself (with cooldown).

### Helmet Item
* Adds a visual component to character and a gameplay effect (e.g. +10% movement speed).

### System Design for a Backend-Authoritative Item System
* Write a few bullet points or class diagram about how a server-side persistent item system might work in an extractor-style or online persistent game.

### World Item Interaction
* Use Lyra’s `InteractionComponent` to:
    * Pick up items from the ground.
    * Drop them for other players to see/pick up.

## Notes
* Please don’t spend more than a few focused hours unless you want to.
* Clean code and a clear modular structure is just as important as polish or flashiness.
* Feel free to write assumptions or stub missing bits as long as the structure is there. Focus on creating correctly the core stuff that matter, not everything.