# Глава 10. GCC и Clang

## Зачем эта тема

На Fedora стоят оба. Для **TSan** исторически **Clang** был эталоном; **GCC** догнал на x86_64. Для **ASan/UBSan** оба пригодны. Знание «чем собрал» объясняет разный текст отчёта и редкие расхождения.

## Переключить компилятор в CMake

```bash
cmake -S multithread -B build-clang-tsan \
  -DCMAKE_CXX_COMPILER=clang++ \
  -DCMAKE_C_COMPILER=clang \
  -DCMAKE_BUILD_TYPE=Debug \
  -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1" \
  -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
```

`clang++` тянет **libstdc++** от GCC по умолчанию — sanitizer runtime от Clang, STL от GCC; для курса нормально.

## libc++ (не по умолчанию на Fedora)

`-stdlib=libc++` — другая STL, другие пути; **не** нужно для `training` без явной цели.

## Версии

```bash
g++ --version
clang++ --version
```

Старый GCC может не собрать TSan — обнови Fedora или используй clang из `dnf install clang`.

## Отчёты

Формат похож; номера frame и demangling могут отличаться. Ищи свой `.cpp` в стеке одинаково.

## Когда второе мнение

Зелёный GCC TSan, подозрение на false positive — прогон clang++ TSan на том же коде. И наоборот.

## MSVC

Sanitizers на Windows другие (ASan в recent VS); курс на Linux — том 5 глава 14.

## dnf update

New compiler — retest TSan once — regression rare but possible.

## LTO + sanitizer

Generally avoid mixing — tom 5 LTO chapter — separate configs.

## std versions

`-std=c++17` both compilers — tom 3/5.

## Report diff example

Frame numbers differ — same file:line — enough.

## Package clang libasan

Usually with clang; glibc host.

## Which to default

Personal: g++ daily, clang++ second opinion for TSan if g++ weird.

## Compiler bug suspicion

File Fedora/gcc or llvm issue — extreme last step.

## Book builds mdbook

Sanitizer not needed for `mdbook build` — tom 0.

## Типичные заблуждения

«clang только для format» — полноценный TSan. «g++ не умеет TSan» — умеет на новых версиях. «Смешать gcc compile и clang link» — нельзя.

## Fedora default: libstdc++

**clang++** on Fedora typically uses **GCC's libstdc++**. TSan runtime from Clang + STL from GCC — **OK** for `training`.

## Switch compiler

```bash
cmake -S multithread -B build-clang-tsan   -DCMAKE_CXX_COMPILER=clang++   -DCMAKE_C_COMPILER=clang   -DCMAKE_CXX_FLAGS="-fsanitize=thread -g -O1"   -DCMAKE_EXE_LINKER_FLAGS="-fsanitize=thread"
```

## TSan history

Clang TSan **mature** first. GCC TSan on x86_64 — check **version**. If `unsupported`-style error — use clang++.

## ASan/UBSan parity

Both compilers fine for **address** and **undefined** on Fedora x86_64.

## Demangling

Reports show `std::...` — both demangle with `-g`.

## Second opinion workflow

GCC TSan green but suspicion remains → clang++ TSan same target. Divergence → read both reports.

## libc++ on Fedora

`-stdlib=libc++` — **not** default; skip unless explicit goal.

## Cannot mix link

**g++**-compiled `.o` with **clang++** link — **don't**. One compiler per executable.

## MSVC (tom 5 ch14)

TSan course = **Linux GCC/Clang**. Windows → WSL/Fedora.

## § К собесу

clang++ on Fedora uses libstdc++; TSan check version; one compiler per link.

<!-- vol7-pass5-ch10 -->

## Дополнительное закрепление

Версии g++/clang++. TSan smoke на **02** вторым compiler.

clang + libstdc++ на Fedora — OK.

Не mix .o compilers.

## Краткое резюме

Fedora: g++ или clang++ с теми же `-fsanitize`. CMake `CXX_COMPILER`. TSan проверь на своей версии. Дальше — suppressions.

## which g++ / which clang++

Multiple versions — use full path in cmake `-DCMAKE_CXX_COMPILER=/usr/bin/g++`.

## sanitizer lib path

Rare link error — `LIBRARY_PATH` — ask tutor if hit on exotic install.

## Report wording GCC vs Clang

«SUMMARY» vs «WARNING» prefixes — both valid; read body.

## Same flags copy-paste

Tom 5 ch13 one-liner — swap g++ → clang++ test once.

<!-- vol7-pass6 -->
## Строка для памяти (Kindle)

Перед сном одним взглядом: **compile+link**, **-g -O1**, **TSan+pthread**, **два build dir**, **два стека в отчёте**, **solutions не спойлерить**. Завтра — один TSan-прогон на 02 или на последнем изменённом advanced target.

## Вопросы для самопроверки

1. Как задать clang++ в CMake?
2. Какая STL по умолчанию у clang на Fedora?
3. Зачем два компилятора для TSan?
4. Где смотреть версию?
5. Нужен ли libc++ для multithread?

<!-- vol7-pass4-ch10 -->
## Fedora практика

Держи установленными **и** `gcc-c++`, **и** `clang` — dnf одной строкой при setup машины (tom 5 ch2). TSan вторым компилятором — **5 минут** verify, не проект.

Не смешивай `.o` от g++ и clang++ в одном link — tom 5 ODR/link chapter echo. Sanitizer не отменяет правило **один compiler per exe**.

Если interview на Linux — «TSan на glibc с pthread» звучит профессионально; «я только Windows без WSL» — честно, но курс `training` эталон Linux.

## Обсуждение с репетитором

**Том:** 7 · **Глава:** `ch10` · **Файл:** `ch10-gcc-vs-clang.md`

**Дальше по курсу:** [Глава 11. Suppressions](./ch11-suppressions.md)
