/**
 *  Koloboks Breeding Game ALPHA (based on SimpleAssets) 
 *  (C) 2019 by CryptoLions [ https://CryptoLions.io ]
 *
 *  Demo:
 *    Jungle Testnet: https://jungle.kolobok.cryptolions.io
 *    EOS Mainnet: -
 *
 *  SimpleAssets:
 *    WebSite:        https://simpleassets.io
 *    GitHub:         https://github.com/CryptoLions/SimpleAssets
 *    Presentation:   https://medium.com/@cryptolions/introducing-simple-assets-b4e17caafaa4
 *
 * SimpleMarket:
 *    WebSite:        https://alpha.simplemarket.io
 *
 */


/**
*  MIT License
* 
* Copyright (c) 2019 Cryptolions.io
* 
* Permission is hereby granted, free of charge, to any person obtaining a copy
* of this software and associated documentation files (the "Software"), to deal
* in the Software without restriction, including without limitation the rights
* to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
* copies of the Software, and to permit persons to whom the Software is
* furnished to do so, subject to the following conditions:
* 
* The above copyright notice and this permission notice shall be included in all
* copies or substantial portions of the Software.
* 
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
* IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
* FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
* AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
* LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
*/

#include <KolobokGame.hpp>
#include <json.hpp>
#include <eosiolib/transaction.hpp>

using json = nlohmann::json;


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
 /*
* Ticker Action
* Is called by this contract only in case there is work  
*/
ACTION KolobokGame::ticker() {
	//only this contract can call this action
	require_auth(get_self());

	tick();
}

//For 0 generation
ACTION KolobokGame::genesis( name owner, string name ) {
	
	require_auth( owner );
	require_recipient(owner);
	
	check (owner == get_self(), "Sorry, only contract owner can create generation 0");
	
	check(name.length() >= 3, "Please eneter a name  min 3 character!");
	check(name.length() <= 20, "Name cannot be longer 20 chars");


	int bn =  tapos_block_num();
	int bp = tapos_block_prefix();
	uint64_t seed1 = now() - bp + bn ;
	
	string  seed = name + std::to_string(seed1);
	
	capi_checksum256 genome;
	sha256( (char *)&seed, sizeof(seed)*2, &genome);

	// Creating asset
	action createAsset = action(
		permission_level{get_self(),"active"_n},
		SIMPLEASSETS_CONTRACT,
		"create"_n,
		std::make_tuple(
			get_self(),
			"kolobok"_n,
			get_self(),
			std::string("{\"gen\":0,\"bdate\":"+std::to_string(now())+",\"p1\":0,\"p2\":0,\"genome\":\""+SHA256toHEX(genome)+"\",\"img\":\"http://5.9.19.86:883/"+SHA256toHEX(genome)+".png\"}"),
			std::string("{\"name\":\""+name+"\",\"kids\":0,\"health\":100,\"cd\":0}"),  //cd - cooldown
			0
		)
	);
	createAsset.send();	

	//SendAllOwnToMarket();

	checkTicker();
}




void KolobokGame::breed(name owner, uint64_t parent1, uint64_t parent2, string name){
	
	
	require_auth(owner);
	
	check(name.length() >= 3, "Please eneter a name  min 3 character!");
	check(name.length() <= 20, "Name cannot be longer 20 chars");


	sassets assets(SIMPLEASSETS_CONTRACT, owner.value);
	auto idxp1 = assets.find(parent1);
	auto idxp2 = assets.find(parent2);
	
	check(parent1 != parent2, "Sorry, no asexual reproduction.");
	
	check(idxp1 != assets.end(), "Parent1 not found or not yours");
	check(idxp2 != assets.end(), "Parent2 not found or not yours");
	
	check (idxp1->author == get_self(), "Parent1 is not Kolobok");
	check (idxp2->author == get_self(), "Parent2 is not Kolobok");

	auto p1_idata = json::parse(idxp1->idata);
	auto p1_mdata = json::parse(idxp1->mdata);

	auto p2_idata = json::parse(idxp2->idata);
	auto p2_mdata = json::parse(idxp2->mdata);

	
	//print (p1->genome, " : ", p2->genome);
	check(p1_mdata["cd"] < now(), "Parent1 not ready for breeding yet.");
	check(p2_mdata["cd"] < now(), "Parent2 not ready for breeding yet.");
	


	int kids_count = (uint64_t)p1_mdata["kids"] + (uint64_t)p2_mdata["kids"];
	uint64_t cdb = cdb_default + kids_count * 60*60*6;
	if (kids_count <=3 )
		cdb = cdb_default + kids_count * 60*60*1;


	//UPD P1 Asset: coldown = now() + p1_idata.cdb
	p1_mdata["cd"] = now() + cdb;
	string p1_mdata_new = p1_mdata.dump();
	updateAsset(owner, parent1, p1_mdata_new);
	
	//UPD P2 Asset: coldown = now() + p2_idata.cdb
	p2_mdata["cd"] = now() + cdb;
	string p2_mdata_new = p2_mdata.dump();
	updateAsset(owner, parent2, p2_mdata_new);


	//+ p2_idata["cdb"])/2;
	babys nbabys(_self, _self.value);
	nbabys.emplace(owner, [&]( auto& g ) {
		g.parent1 = parent1;
		g.parent2 = parent2;
		g.coldown = now() + cdb;
		g.bname = name;
		g.owner = owner;
		
	});
	
	checkTicker();
}

void KolobokGame::updateAsset(name owner, uint64_t assetID, string datam){
	action saUpdate = action(
		permission_level{get_self(),"active"_n},
		SIMPLEASSETS_CONTRACT,
		"update"_n,
		std::make_tuple(_self, owner, assetID, datam)
	);
	saUpdate.send();
	
}

ACTION KolobokGame::borna() {
	require_auth(get_self());
	born();
}
	

uint32_t KolobokGame::born(){

	int PARTS_BY = 4;
	
	babys nbabys(_self, _self.value);	
	uint64_t count = 0;
	uint64_t countWaint = 0;
	
	int index_global = 0;
	int index_part = 0;
	int index_int = 0;

	for(auto itr = nbabys.begin(); itr != nbabys.end() && count!=50;) {
        count++;		
        
		
		int coldown = now() - itr->coldown + 1;
		if (coldown > 0 ) {


			sassets assets(SIMPLEASSETS_CONTRACT, itr->owner.value);
			auto idxp1 = assets.find(itr->parent1);
			auto idxp2 = assets.find(itr->parent2);
			//print("p1: ", idxp1->id, "p2: ",idxp2->id);
			//check (false, "tut");

			//check(idxp1 != assets.end(), "Parent1 not found or not owners");
			//check(idxp2 != assets.end(), "Parent2 not found or not owners");
			if (idxp1 == assets.end() || idxp2 == assets.end()) { 
				itr = nbabys.erase(itr);
		
			} else {		
	
			//if (idxp1 != assets.end() && idxp2 != assets.end()) { 
			
				auto p1_idata = json::parse(idxp1->idata);
				auto p1_mdata = json::parse(idxp1->mdata);

				auto p2_idata = json::parse(idxp2->idata);
				auto p2_mdata = json::parse(idxp2->mdata);

				
				capi_checksum256	new_genome;
				capi_checksum256	p1g = HEX2SHA256((string)p1_idata["genome"]);
				capi_checksum256	p2g = HEX2SHA256((string)p2_idata["genome"]);

				//creating new Genome
				//*********************************************************************
				//*********************************************************************
				//*****                                                ****************
				//*****     Secret breeding logic removed              ****************
				//*****     To make game more interesting              ****************
				//*****                                                ****************
				//*********************************************************************
				//**********************************************************************/


				auto generation = (((int)p1_idata["gen"] + 1 ) + ((int)p2_idata["gen"] + 1 )) / 2;
				//if ((int)p1_idata["gen"] == 0 || (int)p2_idata["gen"] == 0)
				//	generation = 1;

				
				
				uint64_t agep1 = (now() - (uint64_t)p1_idata["bdate"]) / 86400;
				uint64_t agep2 = (now() - (uint64_t)p2_idata["bdate"]) / 86400;
				
		
				int mAge1 = 60 * 60 * 4; //4h if > 30 days
				int mAge2 = 60 * 60 * 4; //4h if > 30 days
				
				if ( agep1 <= 30 ) mAge1 = 60 * 60 * 2; //2h if <= 30 days
				if ( agep1 <= 10 ) mAge1 = 60 * 60 * 1; //1h if <= 10 days
				if ( agep2 <= 30 ) mAge2 = 60 * 60 * 2; //2h if <= 30 days
				if ( agep2 <= 10 ) mAge2 = 60 * 60 * 1; //1h if <= 10 days


				uint64_t cdap1 = cdap_default + agep1 * mAge1 - (uint64_t)p1_mdata["kids"] * 60 * 1;
				uint64_t cdap2 = cdap_default + agep2 * mAge2 - (uint64_t)p2_mdata["kids"] * 60 * 1;
				uint64_t cdab = now() + cdab_default + generation * 60*60*1; //1h for each generation

				p1_mdata["kids"] = (int)p1_mdata["kids"] + 1;
				p1_mdata["cd"] = now() + cdap1;
				
				p2_mdata["kids"] = (int)p2_mdata["kids"] + 1;
				p2_mdata["cd"] = now() + cdap2;
				
				//string p1_mdata_new = p1_mdata.dump();
				//string p2_mdata_new = p2_mdata.dump();
				
				updateAsset(itr->owner, itr->parent1, p1_mdata.dump());
				updateAsset(itr->owner, itr->parent2, p2_mdata.dump());

	
				action createAsset = action(
					permission_level{get_self(),"active"_n},
					SIMPLEASSETS_CONTRACT,
					"create"_n,
					std::make_tuple(
						get_self(),
						"kolobok"_n,
						itr->owner,
						std::string("{\"gen\":"+std::to_string(generation)+",\"bdate\":"+std::to_string(now())+",\"p1\":"+std::to_string(itr->parent1)+",\"p2\":"+std::to_string(itr->parent2)+",\"genome\":\""+SHA256toHEX(new_genome)+"\",\"img\":\"http://5.9.19.86:883/"+SHA256toHEX(new_genome)+".png\"}"), 
						std::string("{\"name\":\""+itr->bname+"\",\"kids\":0,\"health\":100,\"cd\":"+ std::to_string(cdab)+"}"),  //cd - cooldown
						1
					)
				);
				
				createAsset.send();	
				
				itr = nbabys.erase(itr);
	
		
			//} else {
			//	itr = nbabys.erase(itr);
			}
				
		} else {
			countWaint++;
			itr++;
		}
		
		

	}
	
	return countWaint;
	
	


	/* remove all
	babys nbabys(_self, _self.value);	
	uint64_t count = 0;
	for(auto itr = nbabys.begin(); itr != nbabys.end() && count!=10;) {
        itr = nbabys.erase(itr);
        count++;
    }
	*/
}





void KolobokGame::SendAllOwnToMarket() {
	sassets assets(SIMPLEASSETS_CONTRACT, _self.value);
	auto auth_index = assets.template get_index<"author"_n>();
	auto itr = auth_index.find( get_self().value );

	//auto itr = assets.find();
	//check(	itr != assets.end(), "nothing to transfer yet");
	
	for (; itr != auth_index.end(); itr++) {
		auto as = *itr;
		
		//check(gm.challenger.value != challenger.value, "There is open game with this user plese finish existing game first.");		
		
		string memo_tb = "{\"price\":\"4.2000 EOS\"}";
		action sendAsset = action(
			permission_level{get_self(),"active"_n},
			SIMPLEASSETS_CONTRACT,
			"transfer"_n,
			std::make_tuple(get_self(), SIMPLEMARKET_CONTRACT, as.id, memo_tb)
		);

		sendAsset.send();
		
	}
}


void KolobokGame::checkTicker(){
	conf config(_self, _self.value);
	_cstate = config.exists() ? config.get() : global{};
	
	int timedftx = now() - _cstate.lastticker - _cstate.tickertime;

	if (timedftx > 0){
		//deftx(2);	
		tick();
	}

}

void KolobokGame::tick() {
	
	conf config(_self, _self.value);
	_cstate = config.exists() ? config.get() : global{};

	uint32_t work_count = born();
	if (work_count > 0){
		deftx(0);
		//update configs table
		_cstate.lastticker = now();
		config.set(_cstate, _self);
	}

}



//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
/*
* Creating deferred transaction to call ticker action
*/
void KolobokGame::deftx(uint64_t delay ) {
	if (delay<1) 
		delay = _cstate.tickertime;

	//send new one
	transaction tickerTX{};
	tickerTX.actions.emplace_back( action({get_self(), "active"_n}, _self, "ticker"_n, now()) );
	tickerTX.delay_sec = delay;
	tickerTX.expiration = time_point_sec(now() + 10); 
	uint128_t sender_id = (uint128_t(now()) << 64) | now()-5;
	tickerTX.send(sender_id, _self); 
}


//=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-
//------------------------------
//TOOLS for SHA256 convertation
//----------------------------
string KolobokGame::SHA256toHEX(capi_checksum256 sha256) {
	return conv2HEX((char*)sha256.hash, sizeof(sha256.hash));
}

string KolobokGame::conv2HEX(char* hasha, uint32_t ssize) {  
	std::string res;
	const char* hex = "0123456789abcdef";

	uint8_t* conv = (uint8_t*)hasha;
	for (uint32_t i = 0; i < ssize; ++i)
		(res += hex[(conv[i] >> 4)]) += hex[(conv[i] & 0x0f)];
	return res;
}

capi_checksum256 KolobokGame::HEX2SHA256(string hexstr) {
	check(hexstr.length() == 64, "invalid sha256");

	capi_checksum256 cs;
	convFromHEX(hexstr, (char*)cs.hash, sizeof(cs.hash));
	return cs;
}

uint8_t KolobokGame::convFromHEX(char ch) {
	if (ch >= '0' && ch <= '9') return ch - '0';
	if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
	if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;

	check(false, "Wrong hex symbol");
	return 0;
}

size_t KolobokGame::convFromHEX(string hexstr, char* res, size_t res_len) {
	auto itr = hexstr.begin();

	uint8_t* rpos = (uint8_t*)res;
	uint8_t* rend = rpos + res_len;

	while (itr != hexstr.end() && rend != rpos) {
		*rpos = convFromHEX((char)(*itr)) << 4;
		++itr;
		if (itr != hexstr.end()) {
			*rpos |= convFromHEX((char)(*itr));
			++itr;
		}
		++rpos;
	}
	return rpos - (uint8_t*)res;
}


//------------------------------------------------------------------------------



EOSIO_DISPATCH( KolobokGame, (genesis)(breed)(ticker)(borna) )
