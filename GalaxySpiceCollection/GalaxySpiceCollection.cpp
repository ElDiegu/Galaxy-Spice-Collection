#include "stdafx.h"
#include "GalaxySpiceCollection.h"

using namespace Simulator;

bool isNewSystem = false;
const int MAX_SPICE = 99;

GalaxySpiceCollection::GalaxySpiceCollection()
{
}


GalaxySpiceCollection::~GalaxySpiceCollection()
{
}

// For internal use, do not modify.
int GalaxySpiceCollection::AddRef()
{
	return DefaultRefCounted::AddRef();
}

// For internal use, do not modify.
int GalaxySpiceCollection::Release()
{
	return DefaultRefCounted::Release();
}

// You can extend this function to return any other types your class implements.
void* GalaxySpiceCollection::Cast(uint32_t type) const
{
	CLASS_CAST(Object);
	CLASS_CAST(GalaxySpiceCollection);
	return nullptr;
}

void GalaxySpiceCollection::Update()
{
	////// Checks

	// We check if the player is in the SpaceGame, this meaning, that is currently playing in space stage
	if (!IsSpaceGame()) return;

	// We check if the player is in the galaxy context, this meaning, that the player is currently moving around the galaxy
	// and not inside a planet or star
	if (GetCurrentContext() != SpaceContext::Galaxy) return;

	// We check if the player UFO is currently at it's destination
	// We also set the bool variable "isNewSystem", as not beeing at the destination means that 
	// the player has moved towards a new system
	if (!GetPlayerUFO()->mbAtDestination)
	{
		isNewSystem = true;
		return;
	}

	// "isNewSystem" sets to true every time tha player has moved from one system to another, so the player won't
	// collect from the planet unless they move out and in from it, mimicking the Vanilla behaviour
	if (!isNewSystem) return;

	// We get the active StarRecord and check if the star is owned by the player's empire
	cStarRecordPtr currentStar = GetActiveStarRecord();

	if (GetPlayerEmpireID() != currentStar->mEmpireID) return;

	////// Once we've done all the needed checks we begin to calculate the spice to obtain

	vector<cPlanetRecordPtr> starPlanetRecords = GetActiveStarRecord()->GetPlanetRecords();

	// We first iterate through all the planets in the star
	for (int i = 0; i < starPlanetRecords.size(); i++)
	{
		cPlanetRecordPtr planetRecord = starPlanetRecords[i];

		cPlanetPtr planet;
		cStarManager::Get()->RecordToPlanet(planetRecord.get(), planet);

		// We now check if that selected planet is colonized by ANY empire, the planet's GetEmpire pointer
		// is null in case there is no empire present at the planet
		if (planet->GetEmpire() == nullptr) continue;

		// If the planet doesn't belong to the player, we skip the planet
		if (GetPlayerEmpireID() != planet->GetEmpire()->GetEmpireID()) continue;

		// If the planet belong to the player, we iterate through all the colonies that planet has and store it's
		// produced spice in a float variable "spice"
		float spice = 0.0f;

		for (int j = 0; j < planetRecord->mCivData[0]->mCities.size(); j++)
		{
			cCityData* city = planetRecord->mCivData[0]->mCities[j];
			spice += city->mSpiceProduction;
		}

		// If the spice produced is equal or less (?) than 0, then we have nothing to collect
		if (static_cast<int>(spice) <= 0) continue;

		// In this section, we check how many spice the player already has in the inventory, as we cannot give them
		// more than the max spice per cargo space. In Vanilla this number is 99
		cSpaceInventoryItemPtr item = nullptr;
		int itemCount = 0;
		if (GetItemByKey(planetRecord->mSpiceGen, item)) itemCount = item->mItemCount;

		// This method gives the spice to the player and reduces the planet's spice reserve by the same amount
		ReceiveSpice(itemCount, spice, planetRecord->mSpiceGen, planetRecord->mCivData[0]->mCities);
	}

	//If we are here, it means that we've at least visited a valid star, so we set the flag "isNewSystem" to false
	isNewSystem = false;
}

void GalaxySpiceCollection::PlayEffects()
{
	// SPG_Spice_Autocollect_Particles	- Particles
	// SPG_Spice_Autocollect_Info		- Text
	IVisualEffectPtr spicePickupEffect = nullptr;
	if (EffectsManager.CreateVisualEffect(id("SPG_Spice_Autocollect_Particles"), 0, spicePickupEffect))
	{
		spicePickupEffect->SetSourceTransform(Transform().SetOffset(GetPlayerUFO()->mPosition).SetScale(1.0f));
		spicePickupEffect->Start();
	}

	IVisualEffectPtr greenParticlesEffect = nullptr;
	if (EffectsManager.CreateVisualEffect(id("SPG_Spice_Autocollect_Info"), 0, greenParticlesEffect))
	{
		greenParticlesEffect->SetSourceTransform(Transform().SetOffset(GetPlayerUFO()->mPosition).SetScale(1.0f));
		greenParticlesEffect->Start();
	}
}

void GalaxySpiceCollection::ReceiveSpice(int spiceStored, int spiceReceived, ResourceKey spice, vector<cCityData*> cities)
{
	if (spiceStored + spiceReceived <= MAX_SPICE)
	{
		cSpaceTrading::Get()->ObtainTradingObject(spice, spiceReceived);
		for (int i = 0; i < cities.size(); i++) cities[i]->mSpiceProduction = 0.0f;
		GalaxySpiceCollection::PlayEffects();
	}

	spiceReceived = MAX_SPICE - spiceStored;

	for (int i = 0; i < cities.size(); i++)
	{
		if (spiceReceived <= 0) break;

		int citySpice = static_cast<int>(cities[i]->mSpiceProduction);

		if (citySpice > spiceReceived)
		{
			cSpaceTrading::Get()->ObtainTradingObject(spice, spiceReceived);
			GalaxySpiceCollection::PlayEffects();
			cities[i]->mSpiceProduction -= spiceReceived;
			break;
		}
		else if (citySpice <= spiceReceived)
		{
			cSpaceTrading::Get()->ObtainTradingObject(spice, citySpice);
			GalaxySpiceCollection::PlayEffects();
			cities[i]->mSpiceProduction = 0.0f;
			spiceReceived -= citySpice;
		}
	}
}

bool GalaxySpiceCollection::GetItemByKey(ResourceKey key, cSpaceInventoryItemPtr& dst)
{
	if (dst != nullptr) return false;

	cPlayerInventoryPtr playerInventory = SimulatorSpaceGame.GetPlayerInventory();

	for (int i = 0; i < playerInventory->mInventoryItems.size(); i++)
	{
		if (playerInventory->mInventoryItems[i]->GetItemID() != key) continue;

		dst = playerInventory->mInventoryItems[i];

		return true;
	}

	return false;
}
