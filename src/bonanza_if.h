
/* bonanza側から参照する関数/変数はすべて extern "C" で宣言します。*/
#ifdef __cplusplus
extern "C" {
#endif

#if defined(GODWHALE_SERVER)
extern void CONV init_game_hook();
extern void CONV quit_game_hook();
extern void CONV reset_position_hook(const min_posi_t *posi);
extern void CONV make_move_root_hook(move_t move);
extern void CONV unmake_move_root_hook();
extern void CONV adjust_time_hook(int turn);
extern int CONV server_iterate(tree_t *restrict ptree, int *value,
                               move_t *pvseq, int *pvseq_length);
#endif

#ifdef __cplusplus
}
#endif
