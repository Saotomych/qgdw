// Data descriptions for data conversion one real device into data_types IEC61850


#define MAX_DEVICELINK	32

struct iec_data_type{
	char	*iec_name;
};

struct work_data_type{
	u32	val;
	u16 priority;	// priority to send
	u16 eventsubs;	// subscribe to change data
};

