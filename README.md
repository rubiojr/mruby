# このリポジトリについて

mruby に対して、主に UNIX 環境に依存する機能を追加した fork です。

(English translation is under way: https://github.com/iij/mruby/wiki/README.md.en )

# iij/mrubyの拡張部分

 * 追加したクラス/モジュール
   * Errno::EXXX
   * File: ::open ::umask ::unlink ::delete ::rename ::exist? ::exists?
           ::dirname ::basename ::size #path
   * IO: ::open ::sysopen ::popen ::select
         #close #closed? #each #each_byte #each_line #read #sync #sync=
         #write #to_io
   * IPAddr: ::new #<=> #family #hton #inspect #ipv4? #ipv6? #mask #to\_s #| #~
   * Regexp: ::compile ::last\_match #match
   * TCPSocket: ::new ::open
   * UNIXSocket: #addr ::new ::open #peeraddr
   * SystemCallError

 * 拡張したクラス/モジュール
   * Array: #- #& #| #uniq #uniq! #flatten #flatten!
   * Kernel: #exit #load #require #sleep #system
     * load, require については https://github.com/iij/mruby/wiki/require も参照してください
   * String: #lstrip #rstrip #strip #gsub #gsub! #sub #sub! #scan

 * その他の拡張
   * 正規表現リテラル
   * C + Ruby の両方の言語で実装可能な拡張モジュール

http://iij.github.com/mruby/lib/ からリファレンス形式でも参照できます。
ハイライトされているクラス/メソッドが iij/mruby で利用可能なものです。

# mrbgem

mrbgemの形で実装した機能拡張の多くは、mruby/mrubyでも利用可能です。

* https://github.com/iij/mruby-mtest
   * MiniTestライクなTestingFramework
* https://github.com/iij/mruby-dir (iij/mruby のみサポート)
   * Dir: ::[] ::chdir ::chroot ::glob ::home #path #to\_path #inspect 以外
* https://github.com/iij/mruby-digest
   * Digest::MD5/RIPEMD160/SHA1/SHA256/SHA384/SHA512
   * Digest::HMAC: #reset 以外
* https://github.com/iij/mruby-process
   * Process: ::kill ::pid ::ppid
* https://github.com/iij/mruby-pack
   * Array: #pack
   * String: #unpack
* https://github.com/iij/mruby-syslog
   * Syslog: ::open ::close ::log ::opened? ::ident ::options ::facility
* https://github.com/iij/mruby-env
   * ENV: ::[] ::[]= ::clear ::delete ::inspect ::keys ::size ::store
          ::to\_a ::to\_hash ::to\_s ::values
* https://github.com/iij/mruby-simple-random
   * Kernel: #rand #srand


# ブランチ

 * master : mruby/mruby の master と同じです。毎日同期します。
 * iij : 主な開発ブランチです。master (=mruby/mruby) をベースに
   UNIX プラットホームに依存した拡張を含みます。
 * pr-hogehoge : master から分岐した、pull-request 送信専用のブランチです。
   ひとつの pull-request に対してひとつの pr-hogehoge ブランチを作成します。

# 機能拡張に関連するテスト

UNIX 環境に依存する機能のテストコードは test/posix ディレクトリにあり、
以下のコマンドで実行できます。

```
$ cd test/posix   # test/posix ディレクトリへ移動
$ sh all.sh       # test/posix 以下にある全てのテストを実行
```

 * 各種OSでのテスト結果を http://iij.github.com/mruby/report/ で公開しています

# サンプルアプリケーション

このリポジトリで追加した拡張を利用した mruby アプリケーションのサンプルを
app ディレクトリ以下に置いています。

# ライセンス

オリジナル mruby と同じです。

# 謝辞
 * 正規表現リテラルの実装には、以下のレポジトリのコードを一部流用させていただきました。
    * https://github.com/junjis0203/mruby
