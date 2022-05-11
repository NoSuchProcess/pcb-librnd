typedef struct {
	const char *name;
	rnd_coord_t width, height; /* orientation: landscape */
	rnd_coord_t margin_x, margin_y;
} rnd_media_t;

extern rnd_media_t rnd_media_data[];
extern const char *rnd_medias[];

