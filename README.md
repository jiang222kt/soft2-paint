# ペイントソフトの課題について
以下、ペイントソフトの課題でやったことを簡潔にまとめる。

## 課題1
- 長方形を描くコマンド"rect"と円を描くコマンド"circle"を実装した。

## 課題2
- ファイルに保存されたコマンド履歴を読み込んで絵を描画するコマンド"load"を実装した。
- 引数がないときはhistory.txtの中身が読み込まれ、引数があるときはそのファイルの中身が読み取られる。

## 課題3
- 描画の文字種を変更するコマンド"chpen"を実装した。
- 文字種を変更して描画したあとに"undo"をすると、描画されている図形の文字種がすべて変更後の文字種に統一されてしまうため、reset_canvasの関数を以下のように変更してそれを改善した。
```
void reset_canvas(Canvas *c)
{
    const int width = c->width;
    const int height = c->height;
    memset(c->canvas[0], ' ', width*height*sizeof(char));

    c->pen = '*';
}
```

## 発展課題