#pragma once

#include <Spore\BasicIncludes.h>

#define GalaxySpiceCollectionPtr intrusive_ptr<GalaxySpiceCollection>

class GalaxySpiceCollection 
	: public Object
	, public DefaultRefCounted
	, public App::IUpdatable
{
public:
	static const uint32_t TYPE = id("GalaxySpiceCollection");
	
	GalaxySpiceCollection();
	~GalaxySpiceCollection();

	int AddRef() override;
	int Release() override;
	void* Cast(uint32_t type) const override;

	void Update() override;

	void ReceiveSpice(int spiceStored, int spiceReceived, ResourceKey spice, vector<Simulator::cCityData*> cities);
	static void PlayEffects();

	/// <summary>
	/// Searches for a given key inside the SpaceInventory o the player, returning a pointer to said item. Always
	/// gets the first item of type key in the player inventory.
	/// </summary>
	/// <param name="key">ResourceKey of the item we want to get</param>
	/// <param name="dst">An unitilized pointer where to place the item, if found</param>
	/// <returns>True if an item wth given key is found, false otherwise</returns>
	bool GetItemByKey(ResourceKey key, cSpaceInventoryItemPtr& dst);
};
