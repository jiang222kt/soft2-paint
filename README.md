# ペイントソフトの課題について
以下、ペイントソフトの課題でやったことを簡潔にまとめる。

## 課題1
- 長方形を描くコマンド`rect`と円を描くコマンド`circle`を実装した。

## 課題2
- ファイルに保存されたコマンド履歴を読み込んで絵を描画するコマンド`load`を実装した。
- 引数がないときは`history.txt`の中身が読み込まれ、引数があるときはそのファイルの中身が読み取られる。

## 課題3
- 描画の文字種を変更するコマンド`chpen`を実装した。
- 文字種を変更して描画したあとに`undo`をすると、描画されている図形の文字種がすべて変更後の文字種に統一されてしまうため、`reset_canvas`の関数を以下のように変更してそれを解消した。
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
### 色変更
- 文字種の色を変更できるようにした。`chpen`のようにコマンドを入力したあとに色の変更が反映されるようにしたかったが、`chpen`のようなコードを書くと色を変更した途端、その前に描画したものまで変更した後の色に統一されてしまう。これを、Canvas内の共通の属性として色を持たせるのではなく、canvasのように画面のセル単位に色の属性を持たせることによって解消した。

- 入力するコマンドと、出力される色の対応表を以下に記す。

| コマンド | 色 |
----|---- 
| chcolor black | 黒色 |
| chcolor red | 赤色 |
| chcolor green | 緑色 | 
| chcolor yellow | 黄色 |
| chcolor blue | 青色 |
| chcolor magenta | マゼンダ |
| chcolor cyan | シアン |
| chcolor white | 白色 |

### redoの実装
#### 仕様
- `undo`で行った操作を取り消すようなふるまいを`redo`として実装した。
- また、WordやExcelなどのように、`undo`した後に他の操作を挟んで`redo`しようとしてもできないようにした。つまり、`redo`が実行できるのは直前の操作が`undo`のときだけになるようにしたということである。
- さらに、`undo`を複数回連続で行うと、`redo`を連続で行うことによって動作を復元できるようにした。つまり、`undo`を3回行った後に`redo`を3回することができ、このとき`undo`する前の状態に戻すことができる。
#### 実装時のポイント
- `undo`を複数回行った後に`redo`を複数回行うことができるように、`undo_num`というフラッグを`Canvas`の構造体の中に用意し、`undo`するたびに1を増やし、`redo`するたびに1を減らすようにした。上記以外のコマンド(line, rect, circle, chpen, chcolor)を実行した場合、直前の操作が`undo`とならないので、このとき`undo_num`を0にセットした。
  なお、`undo`と`redo`の中には`interpret_command`関数が呼び出されており、毎回`undo_num`が0に更新されてしまうため、一時変数を用意してそれを解消する工夫を行った。
- 従来のコードだと、`undo`でコマンドのためのメモリを解放してしまうため、`undo`の中身を次の考え方に基づいて変えた。まず新たに`redo_his`というredo用のコマンドのリストを保持する変数を定義して初期化した。そして`undo`で`pop_back`したコマンドを`redo`のリストに`push_back`して追加し、`redo`を入力したときにそれを取り出して実行できるようにした。

なお、直前に`undo`を複数回行ったとしても`redo`が一回しか実行できないようなコードも作った。これは`paint5.c`に入っている。