# Secret Santa generator

Provides a small script to setup a "secre santa" game.

## Building

Run: 

```
make SecretSanta
```

## Usage

```
./SecretSanta <input-file> <output-dir>
```

<input-file> contains information about who is in the game, and the constraints (who they cannot give a gift to).
Each person conists of a line with the persons name, followed by a line of names who the person cannot give to.
After that, at least one empty line must follow. For example, the below is a valid input.

```
PersonA
PersonB PersonC PersonD

PersonB
PersonA PersonD

PersonC
PersonB PersonA

PersonD

Person E
```

The above states that there are 5 people in the game:
PersonA, PersonB, PersonC, PersonD and PersonE.

PersonA cannot give to PersonB, PersonC or PersonD.

PersonB cannot give to PersonA or PersonD.

PersonC cannot give to PersonB or PersonA.

PersonD and PersonE can give to everyone.

It is implicitly implied that a person cannot give to themselves. 

## Output

The output of running the script will be a file for each person, with a single name identifying who they should get a gift for.

There will also be a details file that gives the whole overview.