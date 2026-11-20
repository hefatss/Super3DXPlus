# Headline titles

The arcade's menu titles ("Select brand", "Card data confirmed (1/9)") are ready-made pictures, one per string, in a heavy sans with
a red halo. No game font draws that style, so the port's own titles (a new string in the top bar) are drawn by `tools/headline.py`
and baked into `menu.bin` by `tools/menupack.py`.

- `fonts/LibreFranklin[wght].ttf` + `OFL-librefranklin.txt`: the letters (Libre Franklin Bold, 700; SIL Open Font License 1.1).
  `fetch_font.sh` fetches it again from google/fonts.
- `../headline.py`: `render_headline(text)`, the halo model and its constants.
- `test_titles.py`: renders a list of titles to one PNG, to look at them on a dark grey.

## How the halo was found

Fitted against a dump of the arcade's title sprites (the English build's `data/sprite-us/menu_p_topbar_mes_*.png`): a tight red
falling to white, nothing past 6 px, opacity tied to colour (green and blue = 255 - alpha). `render_headline` uses sigma 1.95,
power 0.83, gain 450, tuned by eye against the real ones.
