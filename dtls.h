#ifndef DTLS_H
#define DTLS_H


#include <map>
#include <string>

#include "ip.h"
#include "tools_global.h"

#if defined(HAVE_OPENSSL101) and defined(HAVE_LIBGNUTLS)
#include <openssl/ssl.h>
#else
    #ifndef SSL3_MASTER_SECRET_SIZE
    #define SSL3_MASTER_SECRET_SIZE 48
    #endif
#endif


using namespace std;


#define DTLS_RANDOM_SIZE 32

#define DTLS_CONTENT_TYPE_HANDSHAKE 22

#define DTLS_HANDSHAKE_TYPE_CLIENT_HELLO 1
#define DTLS_HANDSHAKE_TYPE_SERVER_HELLO 2


class cDtlsLink {
public:
	struct sDtlsLinkId {
		vmIPport server;
		vmIPport client;
		sDtlsLinkId(vmIP server_ip, vmPort server_port,
			    vmIP client_ip, vmPort client_port) 
			: server(server_ip, server_port), 
			  client(client_ip, client_port) {
		}
		inline bool operator == (const sDtlsLinkId& other) const {
			return(this->server == other.server &&
			       this->client == other.client);
		}
		inline bool operator < (const sDtlsLinkId& other) const { 
			return(this->server < other.server ||
			       (this->server == other.server && this->client < other.client));
		}
	};
	struct sDtlsServerId {
		vmIPport server;
		sDtlsServerId(vmIP server_ip, vmPort server_port) 
			: server(server_ip, server_port) {
		}
		inline bool operator == (const sDtlsServerId& other) const {
			return(this->server == other.server);
		}
		inline bool operator < (const sDtlsServerId& other) const { 
			return(this->server < other.server);
		}
	};
	struct sSrtpKeys {
		string server_key;
		string client_key;
		vmIPport server;
		vmIPport client;
		string cipher;
	};
	struct sHeader {
		u_int8_t content_type;
		u_int16_t version;
		u_int16_t epoch;
		u_int16_t sequence_number_filler;
		u_int32_t sequence_number;
		u_int16_t length;
		unsigned length_() {
			return(ntohs(length));
		}
	} __attribute__((packed));
	struct sHeaderHandshake {
		u_int8_t handshake_type;
		u_int8_t length_upper;
		u_int16_t length;
		u_int16_t message_sequence;
		u_int8_t fragment_offset_upper;
		u_int16_t fragment_offset;
		u_int8_t fragment_length_upper;
		u_int16_t fragment_length;
		unsigned length_() {
			return(ntohs(length) + (length_upper << 16));
		}
		unsigned fragment_offset_() {
			return(ntohs(fragment_offset) + (fragment_offset_upper << 16));
		}
		unsigned fragment_length_() {
			return(ntohs(fragment_length) + (fragment_length_upper << 16));
		}
		unsigned content_length() {
			return(fragment_length_() ? fragment_length_() : length_());
		}
	} __attribute__((packed));
	struct sHeaderHandshakeHello : public sHeaderHandshake {
		u_int16_t version;
		u_char random[DTLS_RANDOM_SIZE];
	} __attribute__((packed));
	struct sHeaderHandshakeDefragmenter {
		u_int8_t handshake_type;
		unsigned length;
		map<unsigned, SimpleBuffer> fragments;
		bool empty() {
			return(fragments.size() == 0);
		}
		void clear() {
			fragments.clear();
		}
		bool isComplete() {
			unsigned offset = 0;
			for(map<unsigned, SimpleBuffer>::iterator iter = fragments.begin(); iter != fragments.end(); iter++) {
				if(offset != ((sHeaderHandshake*)iter->second.data())->fragment_offset_()) {
					return(false);
				}
				offset += ((sHeaderHandshake*)iter->second.data())->fragment_length_();
			}
			return(offset == length);
		}
		u_char *complete() {
			u_char *hs = new u_char[sizeof(sHeaderHandshake) + length];
			unsigned offset = 0;
			for(map<unsigned, SimpleBuffer>::iterator iter = fragments.begin(); iter != fragments.end(); iter++) {
				if(!offset) {
					memcpy(hs, iter->second.data(), 
					       sizeof(sHeaderHandshake) + ((sHeaderHandshake*)iter->second.data())->fragment_length_());
					offset += sizeof(sHeaderHandshake);
				} else {
					memcpy(hs + offset, iter->second.data() + sizeof(sHeaderHandshake), 
					       ((sHeaderHandshake*)iter->second.data())->fragment_length_());
				}
				offset += ((sHeaderHandshake*)iter->second.data())->fragment_length_();
			}
			return(hs);
		}
	};
	enum eCipherType {
		_ct_na = 0,
		_ct_SRTP_AES128_CM_HMAC_SHA1_80 = 0x0001,
		_ct_SRTP_AES128_CM_HMAC_SHA1_32 =  0x0002,
		_ct_SRTP_NULL_HMAC_SHA1_80 = 0x0005,
		_ct_SRTP_NULL_HMAC_SHA1_32 = 0x0006,
		_ct_SRTP_AEAD_AES_128_GCM = 0x0007,
		_ct_SRTP_AEAD_AES_256_GCM = 0x0008
	};
public:
	cDtlsLink(vmIP server_ip, vmPort server_port,
		  vmIP client_ip, vmPort client_port);
	~cDtlsLink();
	void processHandshake(sHeaderHandshake *handshake);
	bool findSrtpKeys(list<sSrtpKeys*> *keys);
private:
	void init();
	bool findMasterSecret();
	bool cipherTypeIsOK(unsigned ct) {
		return(ct == _ct_SRTP_AES128_CM_HMAC_SHA1_80 ||
		       ct == _ct_SRTP_AES128_CM_HMAC_SHA1_32 ||
		       ct == _ct_SRTP_NULL_HMAC_SHA1_80 ||
		       ct == _ct_SRTP_NULL_HMAC_SHA1_32 ||
		       ct == _ct_SRTP_AEAD_AES_128_GCM ||
		       ct == _ct_SRTP_AEAD_AES_256_GCM);
	}
	bool cipherIsSupported(unsigned ct) {
		return(ct == _ct_SRTP_AES128_CM_HMAC_SHA1_80 ||
		       ct == _ct_SRTP_AES128_CM_HMAC_SHA1_32);
	}
	string cipherName(unsigned ct) {
		return(ct == _ct_SRTP_AES128_CM_HMAC_SHA1_80 ? "AES_CM_128_HMAC_SHA1_80" :
		       ct == _ct_SRTP_AES128_CM_HMAC_SHA1_32 ? "AES_CM_128_HMAC_SHA1_32" :
		       cipherTypeIsOK(ct) ? "unsupported" :"unknown");
	}
	unsigned cipherSrtpKeyLen(unsigned ct) {
		return(ct == _ct_SRTP_AES128_CM_HMAC_SHA1_80 ? 16 :
		       ct == _ct_SRTP_AES128_CM_HMAC_SHA1_32 ? 16 :
		       0);
	}
	unsigned cipherSrtpSaltLen(unsigned /*ct*/) {
		return(14);
	}
private:
	vmIPport server;
	vmIPport client;
	u_char client_random[DTLS_RANDOM_SIZE];
	bool client_random_set;
	u_char server_random[DTLS_RANDOM_SIZE];
	bool server_random_set;
	list<eCipherType> cipher_types;
	u_char master_secret[SSL3_MASTER_SECRET_SIZE];
	u_int16_t master_secret_length;
	u_int16_t keys_block_attempts;
	u_int16_t max_keys_block_attempts;
	sHeaderHandshakeDefragmenter defragmenter;
};

class cDtls {
public:
	cDtls();
	~cDtls();
	bool processHandshake(vmIP src_ip, vmPort src_port,
			      vmIP dst_ip, vmPort dst_port,
			      u_char *data, unsigned data_len);
	bool findSrtpKeys(vmIP src_ip, vmPort src_port,
			  vmIP dst_ip, vmPort dst_port,
			  list<cDtlsLink::sSrtpKeys*> *keys);
private:
	list<cDtlsLink*> links;
	map<cDtlsLink::sDtlsLinkId, cDtlsLink*> links_by_link_id;
	map<cDtlsLink::sDtlsServerId, cDtlsLink*> links_by_server_id;
};


#endif //DTLS_H
