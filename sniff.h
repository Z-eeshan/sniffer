/* Martin Vit support@voipmonitor.org
 * This program is free software, distributed under the terms of
 * the GNU General Public License Version 2.
*/

#ifndef SNIFF_H
#define SNIFF_H

#include <queue>
#include <map>
#include <set>
#include <semaphore.h>

#include "rqueue.h"
#include "voipmonitor.h"
#include "calltable.h"
#include "pcap_queue_block.h"
#include "fraud.h"
#include "tools.h"
#include "audiocodes.h"

#ifdef FREEBSD
#include <machine/endian.h>
#else
#include "asm/byteorder.h"
#endif

#define RTP_FIXED_HEADERLEN 12


void *rtp_read_thread_func(void *arg);
void add_rtp_read_thread();
void set_remove_rtp_read_thread();
int get_index_rtp_read_thread_min_size();
int get_index_rtp_read_thread_min_calls();
double get_rtp_sum_cpu_usage(double *max = NULL);
string get_rtp_threads_cpu_usage(bool callPstat);

#ifdef HAS_NIDS
void readdump_libnids(pcap_t *handle);
#endif

enum eProcessPcapType {
	_pp_read_file = 1,
	_pp_process_calls = 2,
	_pp_dedup = 4,
	_pp_anonymize_ip = 8
};

bool open_global_pcap_handle(const char *pcap, string *error = NULL);
bool process_pcap(const char *pcap_source, const char *pcap_destination, int process_pcap_type, string *error = NULL);
void readdump_libpcap(pcap_t *handle, u_int16_t handle_index, int handle_dlt, PcapDumper *destination, int process_pcap_type);

unsigned int setCallFlags(unsigned long int flags,
			  vmIP ip_src, vmIP ip_dst,
			  const char *caller, const char *called,
			  const char *caller_domain, const char *called_domain,
			  ParsePacket::ppContentsX *parseContents,
			  bool reconfigure = false);

typedef std::map<vmIP, vmIP> nat_aliases_t; //!< 


/* this is copied from libpcap sll.h header file, which is not included in debian distribution */
#define SLL_ADDRLEN       8               /* length of address field */
struct sll_header {
	u_int16_t sll_pkttype;          /* packet type */
	u_int16_t sll_hatype;           /* link-layer address type */
	u_int16_t sll_halen;            /* link-layer address length */
	u_int8_t sll_addr[SLL_ADDRLEN]; /* link-layer address */
	u_int16_t sll_protocol;         /* protocol */
};

struct sll2_header {
	uint16_t sll2_protocol;			/* protocol */
	uint16_t sll2_reserved_mbz;		/* reserved - must be zero */
	uint32_t sll2_if_index;			/* 1-based interface index */
	uint16_t sll2_hatype;			/* link-layer address type */
	uint8_t  sll2_pkttype;			/* packet type */
	uint8_t  sll2_halen;			/* link-layer address length */
	uint8_t  sll2_addr[SLL_ADDRLEN];	/* link-layer address */
};

#define IS_RTP(data, datalen) ((datalen) >= 2 && (htons(*(u_int16_t*)(data)) & 0xC000) == 0x8000)
#define IS_STUN(data, datalen) ((datalen) >= 2 && (htons(*(u_int16_t*)(data)) & 0xC000) == 0x0)
#define IS_DTLS(data, datalen) ((datalen) >= 1 && *(u_char*)data >= 0x14 && *(u_char*)data <= 0x19)
#define IS_DTLS_HANDSHAKE(data, datalen) ((datalen) >= 1 && *(u_char*)data == 0x16)
#define IS_MRCP(data, datalen) ((datalen) >= 4 && ((char*)data)[0] == 'M' && ((char*)data)[1] == 'R' && ((char*)data)[2] == 'C' && ((char*)data)[3] == 'P')

enum e_packet_type {
	_t_packet_sip = 1,
	_t_packet_rtp,
	_t_packet_rtcp,
	_t_packet_dtls,
	_t_packet_mrcp,
	_t_packet_skinny,
	_t_packet_mgcp
};

enum e_packet_s_type {
	_t_packet_s = 1,
	_t_packet_s_stack,
	_t_packet_s_plus_pointer,
	_t_packet_s_process_0,
	_t_packet_s_process
};

struct packet_flags {
	u_int8_t tcp : 2;
	u_int8_t ss7 : 1;
	u_int8_t mrcp : 1;
	u_int8_t ssl : 1;
	u_int8_t skinny : 1;
	u_int8_t mgcp: 1;
	inline void init() {
		for(unsigned i = 0; i < sizeof(*this); i++) {
			((u_char*)this)[i] = 0;
		}
	}
	inline bool other_processing() {
		return(ss7);
	}
	inline bool rtp_processing() {
		return(mrcp);
	}
};

struct packet_s_kamailio_subst {
	vmIP saddr;
	vmIP daddr;
	vmPort source; 
	vmPort dest;
	timeval ts;
	bool is_tcp;
};

struct packet_s {
	u_int8_t __type;
	#if USE_PACKET_NUMBER
	u_int64_t packet_number;
	#endif
	#if not EXPERIMENTAL_PACKETS_WITHOUT_IP
	vmIP _saddr;
	vmIP _daddr; 
	#endif
	u_int32_t _datalen; 
	u_int32_t _datalen_set; 
	pcap_pkthdr *header_pt; 
	const u_char *packet; 
	pcap_block_store *block_store; 
	u_int32_t block_store_index; 
	vmIP sensor_ip;
	u_int16_t handle_index; 
	u_int16_t dlt; 
	u_int16_t sensor_id_u;
	vmPort _source; 
	vmPort _dest;
	u_int16_t _dataoffset;
	u_int16_t header_ip_encaps_offset;
	u_int16_t header_ip_offset;
	sPacketInfoData pid;
	packet_flags pflags;
	#if EXPERIMENTAL_T2_DETACH_X_MOD
	bool skip : 1;
	#endif
	bool need_sip_process : 1;
	#if EXPERIMENTAL_T2_DIRECT_RTP_PUSH
	bool is_rtp : 1;
	#endif
	bool _blockstore_lock : 1;
	bool _packet_alloc : 1;
	#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
	sAudiocodes *audiocodes;
	#endif
	#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
	packet_s_kamailio_subst *kamailio_subst;
	#endif
	inline vmIP saddr_(bool encaps = false) {
		if(encaps) {
			iphdr2 *header_ip = header_ip_(true);
			return(header_ip ?
				header_ip->get_saddr() :
				vmIP(0));
		}
		#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
		if(audiocodes) {
			return(audiocodes->packet_source_ip);
		}
		#endif
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst) {
			return(kamailio_subst->saddr);
		}
		#endif
		#if not EXPERIMENTAL_PACKETS_WITHOUT_IP
		return(_saddr);
		#else
		iphdr2 *header_ip = header_ip_();
		return(header_ip ?
			header_ip->get_saddr() :
			vmIP(0));
		#endif
	}
	inline vmIP *saddr_pt_() {
		#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
		if(audiocodes) {
			return(&audiocodes->packet_source_ip);
		}
		#endif
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst) {
			return(&kamailio_subst->saddr);
		}
		#endif
		return(&_saddr);
	}
	inline vmIP daddr_(bool encaps = false) {
		if(encaps) {
			iphdr2 *header_ip = header_ip_(true);
			return(header_ip ?
				header_ip->get_daddr() :
				vmIP(0));
		}
		#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
		if(audiocodes) {
			return(audiocodes->packet_dest_ip);
		}
		#endif
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst) {
			return(kamailio_subst->daddr);
		}
		#endif
		#if not EXPERIMENTAL_PACKETS_WITHOUT_IP
		return(_daddr);
		#else
		iphdr2 *header_ip = header_ip_();
		return(header_ip ?
			header_ip->get_daddr() :
			vmIP(0));
		#endif
	}
	inline vmIP *daddr_pt_() {
		#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
		if(audiocodes) {
			return(&audiocodes->packet_dest_ip);
		}
		#endif
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst) {
			return(&kamailio_subst->daddr);
		}
		#endif
		return(&_daddr);
	}
	inline vmPort source_() {
		#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
		if(audiocodes) {
			return(audiocodes->packet_source_port);
		}
		#endif
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst) {
			return(kamailio_subst->source);
		}
		#endif
		return(_source);
	}
	inline vmPort dest_() {
		#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
		if(audiocodes) {
			return(audiocodes->packet_dest_port);
		}
		#endif
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst) {
			return(kamailio_subst->dest);
		}
		#endif
		return(_dest);
	}
	inline char *data_() {
		return((char*)(packet + dataoffset_()));
	}
	inline u_int32_t datalen_() {
		if(_datalen_set) {
			return(_datalen_set);
		}
		#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
		if(audiocodes) {
			return(_datalen - audiocodes->get_data_offset((u_char*)(packet + _dataoffset)));
		}
		#endif
		return(_datalen);
	}
	inline void set_datalen_(u_int32_t datalen) {
		if(datalen != datalen_()) {
			_datalen_set = datalen;
		}
	}
	inline u_int32_t dataoffset_() {
		#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
		if(audiocodes) {
			return(_dataoffset + 
			       audiocodes->get_data_offset((u_char*)(packet + _dataoffset)));
		}
		#endif
		return(_dataoffset);
	}
	inline iphdr2 *header_ip_(bool encaps = false) {
		if(encaps) {
			return(header_ip_encaps_offset != 0xFFFF && header_ip_encaps_offset < header_ip_offset ?
				(iphdr2*)(packet + header_ip_encaps_offset) :
				NULL);
		}
		return((iphdr2*)(packet + header_ip_offset));
	}
	inline u_int8_t header_ip_protocol(bool encaps = false) {
		if(encaps) {
			return(header_ip_encaps_offset != 0xFFFF && header_ip_encaps_offset < header_ip_offset ?
				((iphdr2*)(packet + header_ip_encaps_offset))->get_protocol(header_pt->caplen - header_ip_encaps_offset) :
				0xFF);
		}
		return(((iphdr2*)(packet + header_ip_offset))->get_protocol(header_pt->caplen - header_ip_offset));
	}
	inline udphdr2 *header_udp_() {
		iphdr2 *header_ip = header_ip_();
		return((udphdr2*)((char*)header_ip + header_ip->get_hdr_size()));
	}
	inline tcphdr2 *header_tcp_() {
		iphdr2 *header_ip = header_ip_();
		return((tcphdr2*)((char*)header_ip + header_ip->get_hdr_size()));
	}
	inline u_int32_t tcp_seq() {
		if(pflags.tcp) {
			tcphdr2 *header_tcp = header_tcp_();
			if(header_tcp) {
				return(header_tcp->seq);
			}
		}
		return(0);
	}
	inline int sensor_id_() {
		return(sensor_id_u == 0xFFFF ? -1 : sensor_id_u);
	}
	inline packet_s() {
		__type = _t_packet_s;
		init();
	}
	inline u_int64_t getTimeUS() {
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst && isSetTimeval(kamailio_subst->ts)) {
			return(::getTimeUS(kamailio_subst->ts));
		}
		#endif
		return(::getTimeUS(header_pt->ts));
	}
	inline double getTimeSF() {
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst && isSetTimeval(kamailio_subst->ts)) {
			return(::getTimeSF(kamailio_subst->ts));
		}
		#endif
		return(::getTimeSF(header_pt->ts));
	}
	inline timeval getTimeval() {
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst && isSetTimeval(kamailio_subst->ts)) {
			return(kamailio_subst->ts);
		}
		#endif
		return(header_pt->ts);
	}
	inline timeval *getTimeval_pt() {
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst && isSetTimeval(kamailio_subst->ts)) {
			return(&kamailio_subst->ts);
		}
		#endif
		return(&header_pt->ts);
	}
	inline u_int32_t getTime_s() {
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst && isSetTimeval(kamailio_subst->ts)) {
			return(kamailio_subst->ts.tv_sec);
		}
		#endif
		return(header_pt->ts.tv_sec);
	}
	inline u_int32_t getTime_us() {
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst && isSetTimeval(kamailio_subst->ts)) {
			return(kamailio_subst->ts.tv_usec);
		}
		#endif
		return(header_pt->ts.tv_usec);
	}
	inline void init() {
		_blockstore_lock = false;
		_packet_alloc = false;
		#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
		audiocodes = NULL;
		#endif
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		kamailio_subst = NULL;
		#endif
	}
	inline void term() {
		#if not EXPERIMENTAL_SUPPRESS_AUDIOCODES
		if(audiocodes) {
			delete audiocodes;
			audiocodes = NULL;
		}
		#endif
		#if not EXPERIMENTAL_SUPPRESS_KAMAILIO
		if(kamailio_subst) {
			delete kamailio_subst;
			kamailio_subst = NULL;
		}
		#endif
	}
	inline void blockstore_lock(int lock_flag) {
		if(!_blockstore_lock && block_store) {
			block_store->lock_packet(block_store_index, lock_flag);
			_blockstore_lock = true;
		}
	}
	inline void blockstore_forcelock(int lock_flag) {
		if(block_store) {
			block_store->lock_packet(block_store_index, lock_flag);
			_blockstore_lock = true;
		}
	}
	inline void blockstore_setlock() {
		if(block_store) {
			_blockstore_lock = true;
		}
	}
	inline void blockstore_relock(int lock_flag) {
		if(_blockstore_lock && block_store) {
			block_store->lock_packet(block_store_index, lock_flag);
		}
	}
	inline void blockstore_unlock() {
		if(_blockstore_lock && block_store && !is_terminating()) {
			block_store->unlock_packet(block_store_index);
			_blockstore_lock = false;
		}
	}
	inline void blockstore_forceunlock() {
		if(block_store && !is_terminating()) {
			block_store->unlock_packet(block_store_index);
		}
	}
	inline void blockstore_clear() {
		block_store = NULL; 
		block_store_index = 0; 
		_blockstore_lock = false;
	}
	#if DEBUG_SYNC_PCAP_BLOCK_STORE
	inline void blockstore_addflag(int flag) {
	#else
	inline void blockstore_addflag(int /*flag*/) {
	#endif
		#if DEBUG_SYNC_PCAP_BLOCK_STORE
		#if DEBUG_SYNC_PCAP_BLOCK_STORE_FLAGS_LENGTH
		if(_blockstore_lock && block_store) {
			block_store->add_flag(block_store_index, flag);
		}
		#endif
		#endif
	}
	inline void packetdelete() {
		if(_packet_alloc) {
			delete header_pt;
			delete [] packet;
			_packet_alloc = false;
		}
	}
	inline bool isRtp() {
		return(IS_RTP(data_(), datalen_()));
	}
	inline bool isStun() {
		return(IS_STUN(data_(), datalen_()));
	}
	inline bool isDtls() {
		return(!pflags.tcp &&
		       IS_DTLS(data_(), datalen_()));
	}
	inline bool isDtlsHandshake() {
		return(!pflags.tcp &&
		       IS_DTLS_HANDSHAKE(data_(), datalen_()));
	}
	inline bool isMrcp() {
		return(IS_MRCP(data_(), datalen_()));
	}
	inline bool isUdptl() {
		return(!IS_RTP(data_(), datalen_()));
	}
	inline bool okDataLenForRtp() {
		return(datalen_() > 12);
	}
	inline bool okDataLenForUdptl() {
		return(datalen_() > 3);
	}
	inline bool isRtpOkDataLen() {
		return(isRtp() && okDataLenForRtp());
	}
	inline bool isUdptlOkDataLen() {
		return(isUdptl() && okDataLenForUdptl());
	}
	inline bool isRtpUdptlOkDataLen() {
		return(isRtp() ? okDataLenForRtp() : okDataLenForUdptl());
	}
};

struct packet_s_stack : public packet_s {
	cHeapItemsPointerStack *stack;
	inline packet_s_stack() {
		__type = _t_packet_s_stack; 
		init();
	}
	inline void init() {
		packet_s::init();
		stack = NULL;
	}
	inline void term() {
		packet_s::term();
	}
};

struct packet_s_plus_pointer : public packet_s {
	inline packet_s_plus_pointer() {
		__type = _t_packet_s_plus_pointer;
	}
	void *pointer[2];
};

struct packet_s_process_rtp_call_info {
	Call *call;
	int8_t iscaller;
	bool is_rtcp;
	s_sdp_flags sdp_flags;
	bool use_sync;
	bool multiple_calls;
	u_int8_t thread_num_rd;
};

struct packet_s_process_calls_info {
	int length;
	bool find_by_dest;
	#if EXPERIMENTAL_PROCESS_RTP_MOD_01
	u_int8_t threads_rd[MAX_PROCESS_RTP_PACKET_THREADS];
	u_int8_t threads_rd_count;
	#endif
	packet_s_process_rtp_call_info calls[1];
	static unsigned __size_of;
	static inline packet_s_process_calls_info* create() {
		return((packet_s_process_calls_info*) new FILE_LINE(0) u_char[size_of()]);
	}
	static inline void free(packet_s_process_calls_info* call_info) {
		delete [] (u_char*)call_info;
	}
	static inline unsigned size_of() {
		return(__size_of);
	}
	static inline unsigned _size_of() {
		return(sizeof(packet_s_process_calls_info) + 
		       (max_calls() - 1) * sizeof(packet_s_process_rtp_call_info));
	}
	static inline void set_size_of() {
		__size_of = _size_of();
	}
	static inline int max_calls() {
		extern int opt_sdp_multiplication;
		return(max(opt_sdp_multiplication, 1));
	}
};

enum packet_s_process_type_content {
	_pptc_na = 0,
	_pptc_sip = 1,
	_pptc_skinny = 2,
	_pptc_mgcp = 3
};

enum packet_s_process_next_action {
	_ppna_na = 0,
	_ppna_set = 1,
	_ppna_push_to_extend = 2,
	_ppna_push_to_rtp = 3,
	_ppna_push_to_other = 4,
	_ppna_destroy = 5
};

struct packet_s_process_0 : public packet_s_stack {
	volatile u_int8_t use_reuse_counter;
	volatile u_int8_t reuse_counter;
	volatile u_int8_t reuse_counter_sync;
	u_int8_t type_content;
	u_int8_t next_action;
	void *insert_packets;
	#if EXPERIMENTAL_PRECREATION_RTP_HASH_INDEX
	u_int32_t h[2];
	#endif
	packet_s_process_calls_info call_info;
	static unsigned __size_of;
	static inline packet_s_process_0* create() {
		packet_s_process_0 *p = (packet_s_process_0*) new FILE_LINE(0) u_char[size_of()];
		p->create_init();
		return(p);
	}
	static inline void free(packet_s_process_0* call_info) {
		delete [] (u_char*)call_info;
	}
	static inline unsigned size_of() {
		return(__size_of);
	}
	static inline unsigned _size_of() {
		return(sizeof(packet_s_process_0) + 
		       (packet_s_process_calls_info::max_calls() - 1) * sizeof(packet_s_process_rtp_call_info));
	}
	static inline void set_size_of() {
		__size_of = _size_of();
	}
	inline packet_s_process_0() {
		create_init();
	}
	inline void create_init() {
		__type = _t_packet_s_process_0; 
		init();
		init2();
	}
	inline void init() {
		packet_s_stack::init();
		use_reuse_counter = 0;
		insert_packets = NULL;
	}
	inline void init_reuse() {
		if(__type >= _t_packet_s_process_0) {
			use_reuse_counter = 0;
			reuse_counter = 0;
			reuse_counter_sync = 0;
			insert_packets = NULL;
		}
	}
	inline void init2() {
		type_content = _pptc_na;
		next_action = _ppna_na;
		call_info.length = -1;
		init_reuse();
	}
	inline void init2_rtp() {
		call_info.length = -1;
		init_reuse();
	}
	inline void term() {
		packet_s_stack::term();
		if(insert_packets) {
			delete (list<packet_s_process_0*>*)insert_packets;
		}
	}
	inline void new_alloc_packet_header() {
		pcap_pkthdr *header_pt_new = new FILE_LINE(27001) pcap_pkthdr;
		u_char *packet_new = new FILE_LINE(27002) u_char[header_pt->caplen];
		*header_pt_new = *header_pt;
		memcpy(packet_new, packet, header_pt->caplen);
		header_pt = header_pt_new;
		packet = packet_new;
	}
	inline void set_use_reuse_counter() {
		use_reuse_counter = 1;
	}
	inline void set_reuse_counter(u_int8_t inc = 1) {
		use_reuse_counter = 1;
		__sync_add_and_fetch(&reuse_counter, inc);
		if(insert_packets) {
			list<packet_s_process_0*> *insert_packets = (list<packet_s_process_0*>*)this->insert_packets;
			for(list<packet_s_process_0*>::iterator iter = insert_packets->begin(); iter != insert_packets->end(); iter++) {
				(*iter)->set_reuse_counter(inc);
			}
		}
	}
	inline bool is_use_reuse_counter() {
		return(use_reuse_counter);
	}
	inline void reuse_counter_inc_sync(u_int8_t inc = 1) {
		__sync_add_and_fetch(&reuse_counter, inc);
	}
	inline void reuse_counter_dec() {
		--reuse_counter;
	}
	inline void reuse_counter_lock() {
		while(__sync_lock_test_and_set(&reuse_counter_sync, 1));
	}
	inline void reuse_counter_unlock() {
		__sync_lock_release(&reuse_counter_sync);
	}
	inline bool typeContentIsSip() {
		return(type_content == _pptc_sip);
	}
	inline bool typeContentIsSkinny() {
		return(type_content == _pptc_skinny);
	}
	inline bool typeContentIsMgcp() {
		return(type_content == _pptc_mgcp);
	}
	inline void insert_packet(packet_s_process_0 *packet) {
		if(!insert_packets) {
			insert_packets = new FILE_LINE(0) list<packet_s_process_0*>;
		}
		((list<packet_s_process_0*>*)insert_packets)->push_back(packet);
	}
};

struct packet_s_process : public packet_s_process_0 {
	enum eTypeChildPackets {
		_tchp_none,
		_tchp_packet,
		_tchp_list
	};
	ParsePacket::ppContentsX parseContents;
	u_int32_t sipDataOffset;
	u_int32_t sipDataLen;
	char callid[128];
	char *callid_long;
	vector<string> *callid_alternative;
	int sip_method;
	sCseq cseq;
	bool sip_response;
	int lastSIPresponseNum;
	char lastSIPresponse[128];
	bool call_cancel_lsr487;
	Call *call;
	int merged;
	Call *call_created;
	bool _getCallID : 1;
	bool _getSipMethod : 1;
	bool _getLastSipResponse : 1;
	bool _findCall : 1;
	bool _createCall : 1;
	bool _customHeadersDone : 1;
	void *child_packets;
	int8_t child_packets_type;
	inline packet_s_process() {
		__type = _t_packet_s_process; 
		init();
		init2();
	}
	inline packet_s_process& operator = (const packet_s_process& other) {
		memcpy((void*)this, &other, sizeof(*this));
		this->callid_long = NULL;
		this->callid_alternative = NULL;
		this->child_packets = NULL;
		this->child_packets_type = _tchp_none;
		return(*this);
	}
	inline void init() {
		packet_s_process_0::init();
	}
	inline void init_reuse() {
		packet_s_process_0::init_reuse();
		if(__type == _t_packet_s_process) {
			child_packets = NULL;
			child_packets_type = _tchp_none;
		}
	}
	inline void init2() {
		packet_s_process_0::init2();
		sipDataOffset = 0;
		sipDataLen = 0;
		callid[0] = 0;
		callid_long = NULL;
		callid_alternative = NULL;
		sip_method = -1;
		cseq.null();
		sip_response = false;
		lastSIPresponseNum = -1;
		lastSIPresponse[0] = 0;
		call_cancel_lsr487 = false;
		call = NULL;
		merged = 0;
		call_created = NULL;
		_getCallID = false;
		_getSipMethod = false;
		_getLastSipResponse = false;
		_findCall = false;
		_createCall = false;
		_customHeadersDone = false;
		child_packets = NULL;
		child_packets_type = _tchp_none;
	}
	inline void term() {
		packet_s_process_0::term();
		if(callid_long) {
			delete [] callid_long;
			callid_long = NULL;
		}
		if(callid_alternative) {
			delete callid_alternative;
			callid_alternative = NULL;
		}
		if(child_packets && child_packets_type == _tchp_list) {
			delete (list<packet_s_process*>*)child_packets;
		}
	}
	void set_callid(char *callid_input, unsigned callid_length = 0) {
		if(!callid_length) {
			callid_length = strlen(callid_input);
		}
		if(callid_length > sizeof(callid) - 1) {
			callid_long = new FILE_LINE(0) char[callid_length + 1];
			strncpy(callid_long, callid_input, callid_length);
			callid_long[callid_length] = 0;
		} else {
			strncpy(callid, callid_input, callid_length);
			callid[callid_length] = 0;
		}
	}
	void set_callid_alternative(char *callid, unsigned callid_length) {
		if(!callid_alternative) {
			callid_alternative = new FILE_LINE(0) vector<string>;
		}
		char *separator = strnchr(callid, ';', callid_length);
		if(separator) {
			callid_length = separator - callid;
		}
		if(callid_length) {
			bool ok = false;
			for(unsigned i = 0; i < callid_length; i++) {
				if(callid[i] != '0') {
					ok = true;
				}
			}
			if(ok) {
				callid_alternative->push_back(string(callid, callid_length));
			}
		}
	}
	inline char *get_callid() {
		return(callid_long ? callid_long : callid);
	}
	inline u_int8_t get_callid_sipextx_index() {
		extern int preProcessPacketCallX_count;
		char *_callid = callid_long ? callid_long : callid;
		unsigned length = 0;
		while(length < 6 && _callid[length]) {
			++length;
		}
		if(length == 6) {
			return((((unsigned int)_callid[0] * (unsigned int)_callid[1]) ^
				((unsigned int)_callid[2] * (unsigned int)_callid[3]) ^
				((unsigned int)_callid[4] * (unsigned int)_callid[5])) % preProcessPacketCallX_count);
		} else {
			return((unsigned int)_callid[0] % preProcessPacketCallX_count);
		}
	}
	inline void register_child_packet(packet_s_process *child) {
		if(!child_packets) {
			child_packets = child;
			child_packets_type = _tchp_packet;
		} else {
			if(child_packets_type == _tchp_packet) {
				packet_s_process *temp_child = (packet_s_process*)child_packets;
				child_packets = new FILE_LINE(0) list<packet_s_process*>;
				((list<packet_s_process*>*)child_packets)->push_back(temp_child);
				child_packets_type = _tchp_list;
			}
			((list<packet_s_process*>*)child_packets)->push_back(child);
		}
	}
	inline bool is_message() {
		return(sip_method == MESSAGE || cseq.method == MESSAGE);
	}
	inline bool is_register() {
		return(sip_method == REGISTER || cseq.method == REGISTER);
	}
	inline bool is_options() {
		return(sip_method == OPTIONS || cseq.method == OPTIONS);
	}
	inline bool is_notify() {
		return(sip_method == NOTIFY || cseq.method == NOTIFY);
	}
	inline bool is_subscribe() {
		return(sip_method == SUBSCRIBE || cseq.method == SUBSCRIBE);
	}
};


class link_packets_queue {
private:
	struct s_link_id : public d_item<vmIPport> {
	};
	struct s_link {
		u_int64_t first_time_ms;
		u_int64_t last_time_ms;
		list<packet_s*> queue;
	};
public:
	link_packets_queue() {
		sync = 0;
		last_cleanup_ms = 0;
		cleanup_interval_ms = 5000;
		expiration_link_ms = 10000;
		expiration_link_count = 20;
	}
	~link_packets_queue() {
		destroyAll();
	}
	void setExpirationLink_ms(unsigned expiration_link_ms) {
		this->expiration_link_ms = expiration_link_ms;
	}
	void setExpirationLink_count(unsigned expiration_link_count) {
		this->expiration_link_count = expiration_link_count;
	}
	void push(packet_s *packetS) {
		u_int64_t time_ms = getTimeMS_rdtsc();
		lock();
		_cleanup(time_ms);
		s_link_id id;
		createId(&id, packetS);
		s_link *link = NULL;
		map<s_link_id, s_link*>::iterator iter_link = links.find(id);
		if(iter_link != links.end()) {
			link = iter_link->second;
		}
		if(!link) {
			link = new FILE_LINE(0) s_link;
			links[id] = link;
			link->first_time_ms = time_ms;
		}
		link->queue.push_back(packetS);
		link->last_time_ms = time_ms;
		if(!link->first_time_ms) {
			link->first_time_ms = time_ms;
		}
		unlock();
		packetS->blockstore_addflag(121 /*pb lock flag*/);
		#if DEBUG_DTLS_QUEUE
		cout << " * queue dtls" << endl;
		#endif
	}
	void moveToPacket(packet_s_process_0 *packetS) {
		s_link_id id;
		createId(&id, packetS);
		s_link *link = NULL;
		map<s_link_id, s_link*>::iterator iter_link = links.find(id);
		if(iter_link != links.end()) {
			link = iter_link->second;
			for(list<packet_s*>::iterator iter = link->queue.begin(); iter != link->queue.end(); iter++) {
				packetS->insert_packet((packet_s_process_0*)(*iter));
				((packet_s_process_0*)(*iter))->blockstore_addflag(122 /*pb lock flag*/);
				#if DEBUG_DTLS_QUEUE
				cout << " * insert dtls" << endl;
				#endif
			}
			delete link;
			links.erase(iter_link);
		}
	}
	inline bool existsContent() {
		return(links.size());
	}
	inline bool existsLink(packet_s *packetS) {
		s_link_id id;
		createId(&id, packetS);
		return(links.find(id) != links.end());
	}
	void cleanup();
	void _cleanup(u_int64_t time_ms);
	void destroyAll();
	void lock() {
		__SYNC_LOCK_USLEEP(sync, 10);
	}
	void unlock() {
		__SYNC_UNLOCK(sync);
	}
private:
	inline void createId(s_link_id *id, packet_s *packetS) {
		vmIPport src = vmIPport(packetS->saddr_(), packetS->source_());
		vmIPport dst = vmIPport(packetS->daddr_(), packetS->dest_());
		if(src > dst) {
			id->items[0] = src;
			id->items[1] = dst;
		} else {
			id->items[0] = dst;
			id->items[1] = src;
		}
	}
private:
	map<s_link_id, s_link*> links;
	volatile int sync;
	u_int64_t last_cleanup_ms;
	unsigned cleanup_interval_ms;
	unsigned expiration_link_ms;
	unsigned expiration_link_count;
};


void save_packet(Call *call, packet_s *packetS, int type, u_int8_t forceVirtualUdp = false, u_int32_t forceDatalen = 0);
void save_packet(Call *call, packet_s_process *packetS, int type, u_int8_t forceVirtualUdp = false, u_int32_t forceDatalen = 0);


typedef struct {
	Call *call;
	packet_s_process_0 *packet;
	int8_t iscaller;
	char find_by_dest;
	char is_rtcp;
	char stream_in_multiple_calls;
	s_sdp_flags_base sdp_flags;
	char save_packet;
} rtp_packet_pcap_queue;

class rtp_read_thread {
public:
	#if DEBUG_QUEUE_RTP_THREAD
	struct thread_debug_data {
		//packet_s_process_0 packets[1000];
		unsigned counter;
		unsigned process_counter;
		unsigned tid;
	};
	#endif
	struct batch_packet_rtp_base {
		batch_packet_rtp_base(unsigned max_count) {
			batch = new FILE_LINE(0) rtp_packet_pcap_queue[max_count];
			memset(CAST_OBJ_TO_VOID(batch), 0, sizeof(rtp_packet_pcap_queue) * max_count);
			this->max_count = max_count;
		}
		virtual ~batch_packet_rtp_base() {
			for(unsigned i = 0; i < max_count; i++) {
				// unlock item
			}
			delete [] batch;
		}
		rtp_packet_pcap_queue *batch;
		unsigned max_count;
	};
	struct batch_packet_rtp : public batch_packet_rtp_base {
		batch_packet_rtp(unsigned max_count) : batch_packet_rtp_base(max_count) {
			count = 0;
			used = 0;
		}
		volatile unsigned count;
		volatile int used;
	};
	struct batch_packet_rtp_thread_buffer : public batch_packet_rtp_base {
		batch_packet_rtp_thread_buffer(unsigned max_count) : batch_packet_rtp_base(max_count) {
			count = 0;
		}
		unsigned count;
	};
public:
	rtp_read_thread()  {
		this->calls = 0;
	}
	void init(int threadNum, size_t qring_length);
	void init_qring(size_t qring_length);
	void alloc_qring();
	void init_thread_buffer();
	void term();
	void term_qring();
	void term_thread_buffer();
	size_t qring_size();
	inline void push(Call *call, packet_s_process_0 *packet, int iscaller, bool find_by_dest, int is_rtcp, bool stream_in_multiple_calls, s_sdp_flags_base sdp_flags, int enable_save_packet, int threadIndex = 0) {
		
		/* destroy and quit - debug
		void PACKET_S_PROCESS_DESTROY(packet_s_process_0 **packet);
		PACKET_S_PROCESS_DESTROY(&packet);
		return;
		*/
		
		extern int opt_t2_boost;
		if(threadIndex && opt_t2_boost) {
		 
			#if DEBUG_QUEUE_RTP_THREAD
			unsigned tid = get_unix_tid();
			if(!tdd[threadIndex-1].tid) {
				tdd[threadIndex-1].tid = tid;
			} else if(tdd[threadIndex-1].tid != tid) {
				syslog(LOG_NOTICE, "RACE in rtp_read_thread(%i)::push - %u / %u", threadNum, tdd[threadIndex-1].tid, tid);
			}
			#endif
		 
			packet->blockstore_addflag(61 /*pb lock flag*/);
			packet->blockstore_addflag(threadNum /*pb lock flag*/);
			packet->blockstore_addflag(threadIndex /*pb lock flag*/);
			
			batch_packet_rtp_thread_buffer *thread_buffer = this->thread_buffer[threadIndex - 1];
			if(thread_buffer->count == thread_buffer->max_count) {
				while(__sync_lock_test_and_set(&this->push_lock_sync, 1));
				packet->blockstore_addflag(62 /*pb lock flag*/);

				/* destroy threadbuffer array - debug
				for(unsigned i = 0; i < thread_buffer->count; i++) {
					thread_buffer->batch.pt[i].packet->blockstore_addflag(64); // pb lock flag
					void PACKET_S_PROCESS_DESTROY(packet_s_process_0 **packet);
					PACKET_S_PROCESS_DESTROY(&thread_buffer->batch.pt[i].packet);
				}
				goto end_thread_buffer_copy;
				*/
				
				batch_packet_rtp *current_batch = this->qring[this->writeit];
				unsigned int usleepCounter = 0;
				while(current_batch->used != 0) {
					USLEEP_C(20, usleepCounter++);
				}
				memcpy(current_batch->batch, thread_buffer->batch, sizeof(rtp_packet_pcap_queue) * thread_buffer->count);
				#if RQUEUE_SAFE
					__SYNC_SET_TO_LOCK(current_batch->count, thread_buffer->count, this->count_lock_sync);
					__SYNC_SET(current_batch->used);
					__SYNC_INCR(this->writeit, this->qring_length);
				#else
					__SYNC_LOCK(this->count_lock_sync);
					current_batch->count = thread_buffer->count;
					__SYNC_UNLOCK(this->count_lock_sync);
					__sync_add_and_fetch(&current_batch->used, 1);
					if((this->writeit + 1) == this->qring_length) {
						this->writeit = 0;
					} else {
						this->writeit++;
					}
				#endif
				
				/* destroy threadbuffer array - debug
				end_thread_buffer_copy:
				*/
				
				thread_buffer->count = 0;
				__sync_lock_release(&this->push_lock_sync);
			}
			rtp_packet_pcap_queue *rtpp_pq = &thread_buffer->batch[thread_buffer->count];
			rtpp_pq->call = call;
			rtpp_pq->packet = packet;
			rtpp_pq->iscaller = iscaller;
			rtpp_pq->find_by_dest = find_by_dest;
			rtpp_pq->is_rtcp = is_rtcp;
			rtpp_pq->stream_in_multiple_calls = stream_in_multiple_calls;
			rtpp_pq->sdp_flags = sdp_flags;
			rtpp_pq->save_packet = enable_save_packet;
			packet->blockstore_addflag(63 /*pb lock flag*/);
			thread_buffer->count++;
		} else {
			while(__sync_lock_test_and_set(&this->push_lock_sync, 1));
		 
			#if DEBUG_QUEUE_RTP_THREAD
			unsigned tid = get_unix_tid();
			if(!tdd[0].tid) {
				tdd[0].tid = tid;
			} else if(tdd[0].tid != tid) {
				syslog(LOG_NOTICE, "RACE in rtp_read_thread(%i)::push - %u / %u", threadNum, tdd[0].tid, tid);
			}
			#endif
		 
			packet->blockstore_addflag(61 /*pb lock flag*/);
			packet->blockstore_addflag(threadNum /*pb lock flag*/);
			
			if(!qring_push_index) {
				packet->blockstore_addflag(62 /*pb lock flag*/);
				unsigned int usleepCounter = 0;
				while(this->qring[this->writeit]->used != 0) {
					USLEEP_C(20, usleepCounter++);
				}
				qring_push_index = this->writeit + 1;
				qring_push_index_count = 0;
				qring_active_push_item = this->qring[qring_push_index - 1];
			}
			rtp_packet_pcap_queue *rtpp_pq = &qring_active_push_item->batch[qring_push_index_count];
			rtpp_pq->call = call;
			rtpp_pq->packet = packet;
			rtpp_pq->iscaller = iscaller;
			rtpp_pq->find_by_dest = find_by_dest;
			rtpp_pq->is_rtcp = is_rtcp;
			rtpp_pq->stream_in_multiple_calls = stream_in_multiple_calls;
			rtpp_pq->sdp_flags = sdp_flags;
			rtpp_pq->save_packet = enable_save_packet;
			++qring_push_index_count;
			packet->blockstore_addflag(63 /*pb lock flag*/);
			if(qring_push_index_count == qring_active_push_item->max_count) {
				packet->blockstore_addflag(64 /*pb lock flag*/);
				#if RQUEUE_SAFE
					__SYNC_SET_TO_LOCK(qring_active_push_item->count, qring_push_index_count, this->count_lock_sync);
					__SYNC_SET(qring_active_push_item->used);
					__SYNC_INCR(this->writeit, this->qring_length);
				#else
					__SYNC_LOCK(this->count_lock_sync);
					qring_active_push_item->count = qring_push_index_count;
					__SYNC_UNLOCK(this->count_lock_sync);
					qring_active_push_item->used = 1;
					if((this->writeit + 1) == this->qring_length) {
						this->writeit = 0;
					} else {
						this->writeit++;
					}
				#endif
				qring_push_index = 0;
				qring_push_index_count = 0;
			}
			__sync_lock_release(&this->push_lock_sync);
		}
	}
	inline void push_batch() {
		while(__sync_lock_test_and_set(&this->push_lock_sync, 1));
	 
		#if DEBUG_QUEUE_RTP_THREAD
		unsigned tid = get_unix_tid();
		if(!tdd[0].tid) {
			tdd[0].tid = tid;
		} else if(tdd[0].tid != tid) {
			syslog(LOG_NOTICE, "RACE in rtp_read_thread(%i)::push_batch - %u / %u", threadNum, tdd[0].tid, tid);
		}
		#endif
			
		if(qring_push_index_count) {
			#if RQUEUE_SAFE
				__SYNC_SET_TO_LOCK(qring_active_push_item->count, qring_push_index_count, this->count_lock_sync);
				__SYNC_SET(qring_active_push_item->used);
				__SYNC_INCR(this->writeit, this->qring_length);
			#else
				__SYNC_LOCK(this->count_lock_sync);
				qring_active_push_item->count = qring_push_index_count;
				__SYNC_UNLOCK(this->count_lock_sync);
				qring_active_push_item->used = 1;
				if((this->writeit + 1) == this->qring_length) {
					this->writeit = 0;
				} else {
					this->writeit++;
				}
			#endif
			qring_push_index = 0;
			qring_push_index_count = 0;
		}
		__sync_lock_release(&this->push_lock_sync);
	}
	inline void push_thread_buffer(int threadIndex) {
	 
		#if DEBUG_QUEUE_RTP_THREAD
		unsigned tid = get_unix_tid();
		if(!tdd[threadIndex].tid) {
			tdd[threadIndex].tid = tid;
		} else if(tdd[threadIndex].tid != tid) {
			syslog(LOG_NOTICE, "RACE in rtp_read_thread(%i)::push_thread_buffer - %u / %u", threadNum, tdd[threadIndex].tid, tid);
		}
		#endif
			
		batch_packet_rtp_thread_buffer *thread_buffer = this->thread_buffer[threadIndex];
		if(thread_buffer->count) {
		 
			#if DEBUG_QUEUE_RTP_THREAD
			syslog(LOG_NOTICE, "push_thread_buffer in rtp_read_thread(%i) - %i", threadNum, threadIndex);
			#endif
		 
			while(__sync_lock_test_and_set(&this->push_lock_sync, 1));
			batch_packet_rtp *current_batch = this->qring[this->writeit];
			unsigned int usleepCounter = 0;
			while(current_batch->used != 0) {
				USLEEP_C(20, usleepCounter++);
			}
			memcpy(current_batch->batch, thread_buffer->batch, sizeof(rtp_packet_pcap_queue) * thread_buffer->count);
			#if RQUEUE_SAFE
				__SYNC_SET_TO_LOCK(current_batch->count, thread_buffer->count, this->count_lock_sync);
				__SYNC_SET(current_batch->used);
				__SYNC_INCR(this->writeit, this->qring_length);
			#else
				__SYNC_LOCK(this->count_lock_sync);
				current_batch->count = thread_buffer->count;
				__SYNC_UNLOCK(this->count_lock_sync);
				current_batch->used = 1;
				if((this->writeit + 1) == this->qring_length) {
					this->writeit = 0;
				} else {
					this->writeit++;
				}
			#endif
			thread_buffer->count = 0;
			__sync_lock_release(&this->push_lock_sync);
		}
	}
public:
	pthread_t thread;
	volatile int threadId;
	int threadNum;
	unsigned int qring_batch_item_length;
	unsigned int qring_length;
	batch_packet_rtp **qring;
	unsigned int thread_buffer_length;
	batch_packet_rtp_thread_buffer **thread_buffer;
	#if DEBUG_QUEUE_RTP_THREAD
	thread_debug_data *tdd;
	#endif
	batch_packet_rtp *qring_active_push_item;
	volatile unsigned qring_push_index;
	volatile unsigned qring_push_index_count;
	volatile unsigned int readit;
	volatile unsigned int writeit;
	pstat_data threadPstatData[2];
	volatile bool remove_flag;
	u_int32_t last_use_time_s;
	volatile u_int32_t calls;
	volatile int push_lock_sync;
	volatile int count_lock_sync;
};

#define MAXLIVEFILTERS 10
#define MAXLIVEFILTERSCHARS 64

struct livesnifferfilter_s_base {
	struct state_s {
		bool all_saddr;
		bool all_daddr;
		bool all_bothaddr;
		bool all_bothport;
		bool all_addr;
		bool all_srcnum;
		bool all_dstnum;
		bool all_bothnum;
		bool all_num;
                bool all_fromhstr;
                bool all_tohstr;
                bool all_bothhstr;
                bool all_hstr;
                bool all_vlan;
		bool all_siptypes;
		bool all_all;
	};
	livesnifferfilter_s_base() {
		memset((void*)this, 0, sizeof(livesnifferfilter_s_base));
		created_at = time(NULL);
	}
	bool sensor_id_set;
        vmIP lv_saddr[MAXLIVEFILTERS];
        vmIP lv_daddr[MAXLIVEFILTERS];
	vmIP lv_bothaddr[MAXLIVEFILTERS];
        vmIP lv_smask[MAXLIVEFILTERS];
        vmIP lv_dmask[MAXLIVEFILTERS];
	vmIP lv_bothmask[MAXLIVEFILTERS];
	vmPort lv_bothport[MAXLIVEFILTERS];
        char lv_srcnum[MAXLIVEFILTERS][MAXLIVEFILTERSCHARS];
        char lv_dstnum[MAXLIVEFILTERS][MAXLIVEFILTERSCHARS];
	char lv_bothnum[MAXLIVEFILTERS][MAXLIVEFILTERSCHARS];
        char lv_fromhstr[MAXLIVEFILTERS][MAXLIVEFILTERSCHARS];
        char lv_tohstr[MAXLIVEFILTERS][MAXLIVEFILTERSCHARS];
        char lv_bothhstr[MAXLIVEFILTERS][MAXLIVEFILTERSCHARS];
        int lv_vlan[MAXLIVEFILTERS];
	bool lv_vlan_set[MAXLIVEFILTERS];
	unsigned char lv_siptypes[MAXLIVEFILTERS];
        int uid;
	int timeout_s;
	bool disable_timeout_warn_msg;
        time_t created_at;
	state_s state;
	SimpleBuffer parameters;
};

struct livesnifferfilter_s : public livesnifferfilter_s_base {
	livesnifferfilter_s() : livesnifferfilter_s_base() {
	}
	std::set<int> sensor_id;
	void updateState();
	string getStringState();
};

struct livesnifferfilter_use_siptypes_s {
	bool u_invite;
	bool u_register;
	bool u_options;
	bool u_subscribe;
	bool u_message;
	bool u_notify;
};

struct gre_hdr {
#if defined(__LITTLE_ENDIAN_BITFIELD)
#ifdef FREEBSD
        u_int16_t rec:3,
#else
        __u16   rec:3,
#endif
                srr:1,
                seq:1,
                key:1,
                routing:1,
                csum:1,
                version:3,
                reserved:4,
                ack:1;
#elif defined(__BIG_ENDIAN_BITFIELD)
#ifdef FREEBSD
        u_int16_t   csum:1,
#else
	__u16   csum:1,
#endif
                routing:1,
                key:1,
                seq:1,
                srr:1,
                rec:3,
                ack:1,
                reserved:4,
                version:3;
#else
#error "Adjust your <asm/byteorder.h> defines"
#endif
#ifdef FREEBSD
        u_int16_t  protocol;
#else
	__be16	protocol;
#endif
};


#define enable_save_sip(call)		(call->flags & FLAG_SAVESIP)
#define enable_save_register(call)	(call->flags & FLAG_SAVEREGISTER)
#define enable_save_rtcp(call)		((call->flags & FLAG_SAVERTCP) || (call->isfax && opt_saveudptl))
#define enable_save_mrcp(call)		((call->flags & FLAG_SAVEMRCP) || (call->isfax && opt_saveudptl))
#define enable_save_application(call)	(enable_save_mrcp(call))
#define enable_save_rtp_audio(call)	((call->flags & (FLAG_SAVERTP | FLAG_SAVERTPHEADER)) || (call->isfax && opt_saveudptl) || opt_saverfc2833)
#define enable_save_rtp_video(call)	(call->flags & (FLAG_SAVERTP_VIDEO | FLAG_SAVERTP_VIDEO_HEADER))
#define enable_save_rtp_av(call)	(enable_save_rtp_audio(call) || enable_save_rtp_video(call))
#define enable_save_rtp(call)		(enable_save_rtp_audio(call) || enable_save_rtp_video(call) || enable_save_application(call))
#define enable_save_rtp_media(call, flags) \
					(flags.is_audio() ? enable_save_rtp_audio(call) : \
					(flags.is_video() ? enable_save_rtp_video(call) : \
					(flags.is_application() ? enable_save_application(call) : enable_save_rtp_audio(call))))
#define enable_save_rtp_packet(call, type) \
					(type == _t_packet_mrcp ? enable_save_mrcp(call) : enable_save_rtp_av(call))
#define enable_save_sip_rtp(call)	(enable_save_sip(call) || enable_save_rtp(call))
#define processing_rtp_video(call)	(call->flags & (FLAG_SAVERTP_VIDEO | FLAG_SAVERTP_VIDEO_HEADER | FLAG_PROCESSING_RTP_VIDEO))
#define enable_save_packet(call)	(enable_save_sip(call) || enable_save_register(call) || enable_save_rtp(call))
#define enable_save_audio(call)		((call->flags & FLAG_SAVEAUDIO) || opt_savewav_force)
#define enable_save_sip_rtp_audio(call)	(enable_save_sip_rtp(call) || enable_save_audio(call))
#define enable_save_any(call)		(enable_save_packet(call) || enable_save_audio(call))


void trace_call(u_char *packet, unsigned caplen, int pcapLinkHeaderType,
		u_int16_t header_ip_offset, u_int64_t packet_time, 
		u_char *data, unsigned datalen,
		const char *file, unsigned line, const char *function, const char *descr = NULL);

inline u_int32_t get_pcap_snaplen() {
	extern int opt_snaplen;
	extern int opt_enable_http;
	extern int opt_enable_webrtc;
	extern int opt_enable_ssl;
	return((opt_snaplen > 0 ? (unsigned)opt_snaplen :
	       (opt_enable_http || opt_enable_webrtc || opt_enable_ssl ? 6000 : 3200)));
}


#endif
