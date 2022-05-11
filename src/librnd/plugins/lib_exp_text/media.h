typedef struct {
	const char *name;
	rnd_coord_t width, height; /* orientation: landscape */
	rnd_coord_t margin_x, margin_y;

	/* Spare: see doc/developer/spare.txt */
	void (*spare_f1)(void), (*spare_f2)(void);
	long spare_l1, spare_l2;
	void *spare_p1, *spare_p2;
	rnd_coord_t spare_c1, spare_c2;
	double spare_d1, spare_d2;
} rnd_media_t;

extern rnd_media_t rnd_media_data[];
extern const char *rnd_medias[];

