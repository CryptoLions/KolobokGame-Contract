/**
 *  Kolobok Breeding Game (based on SimpleAssets)
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

#include <eosiolib/eosio.hpp>
#include <eosiolib/asset.hpp>
#include <eosiolib/symbol.hpp>
#include <eosiolib/singleton.hpp>

#include <string>

using namespace eosio;
using std::string;

//name SIMPLEASSETS_CONTRACT = "simpleassets"_n; // SimpleAssets CONTRACT
//name SIMPLEMARKET_CONTRACT = "simplemarket"_n; // SimpleAssets CONTRACT

name SIMPLEASSETS_CONTRACT = "simpl1assets"_n; // SimpleAssets CONTRACT
name SIMPLEMARKET_CONTRACT = "simpl1market"_n; // SimpleAssets CONTRACT

uint16_t cdb_default = 60*10; //cdb = coldown breeding; 10min
uint16_t cdap_default = 60*30; //cdap = coldown after born for parents; 30min
uint16_t cdab_default = 60*10; //cdab = coldown after born for kids; 10min

CONTRACT KolobokGame : public contract {
	public:
		using contract::contract;


		ACTION genesis( name owner, string name );
		using genesis_action = action_wrapper<"genesis"_n, &KolobokGame::genesis>;
	  
		ACTION breed(name owner, uint64_t parent1, uint64_t parent2, string name);
		using breed_action = action_wrapper<"breed"_n, &KolobokGame::breed>;
		
		ACTION ticker();
		using ticker_action = action_wrapper<"ticker"_n, &KolobokGame::ticker>;

		ACTION borna();
		using borna_action = action_wrapper<"borna"_n, &KolobokGame::borna>;

	private:	

		void SendAllOwnToMarket();
		uint32_t born ();
		void updateAsset(name owner, uint64_t assetID, string datam);
		void checkTicker();
		void tick();
		void deftx(uint64_t delay );
		
		string SHA256toHEX(capi_checksum256 sha256);
		string conv2HEX(char* hasha, uint32_t ssize);
		capi_checksum256 HEX2SHA256(string hexstr); 
		uint8_t convFromHEX(char ch);
		size_t  convFromHEX(string hexstr, char* res, size_t res_len);	



		
		//tabels
		TABLE baby {
			uint64_t parent1;
			uint64_t parent2;			
			uint64_t coldown;
			string bname;
			name owner;	

			auto primary_key() const { return parent1; }			

			uint64_t by_owner() const {
				return owner.value;
			}
			
			EOSLIB_SERIALIZE( baby, (parent1)(parent2)(coldown)(bname)(owner))
		};
		
		typedef eosio::multi_index< "babys"_n, baby, 
			eosio::indexed_by< "owner"_n, eosio::const_mem_fun<baby, uint64_t, &baby::by_owner> >
			//eosio::indexed_by< "author"_n, eosio::const_mem_fun<baby, uint64_t, &baby::by_author> >
		> babys;	
		
		
		TABLE sasset {
			uint64_t			id;
			name				owner;
			name				author;
			name				category;
			string				idata; // immutable data
			string				mdata; // mutable data
		
			auto primary_key() const {
				return id;
			}
	
			uint64_t by_author() const {
				return author.value;
			}


			EOSLIB_SERIALIZE( sasset, (id)(owner)(author)(category)(idata)(mdata))
		};
		
		typedef eosio::multi_index< "sassets"_n, sasset, 		
				eosio::indexed_by< "author"_n, eosio::const_mem_fun<sasset, uint64_t, &sasset::by_author> >
		> sassets;
		
		
		TABLE global {
			global(){}
			uint64_t tickertime = 60*3;
			uint64_t lastticker = 0;			
			uint64_t spare1 = 0;			
			uint64_t spare2 = 0;			
			EOSLIB_SERIALIZE( global, (tickertime)(lastticker)(spare1)(spare2) )

		};

		typedef eosio::singleton< "global"_n, global> conf;
		global _cstate;
	  
};