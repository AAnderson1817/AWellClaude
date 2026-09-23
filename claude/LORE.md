# LORE — The Hearth

A working document for the team. None of it appears in the game as words (L5); every name
here is ours. If a scene contradicts this file, change one of them, with a reason.

## The idea, in one sentence

**The city is cold and cannot make fire; the antechamber is its hearth, and the treasure
hunters are the fire.** The door shut behind you for the reason anyone shuts a door in
winter.

Nothing here wants to hurt you. Everything here wants to sit near you.

## 0. The archive (the user's direction, after the first build)

The temple is **a living archive of technology interwoven with nature.** This does not
replace the hearth; it is what the hearth is made of. The tall ones' glass keeps what
touches it -- that is already how the prints work: the stone kept each push's warmth and
gives it back cold -- so everything they built is a record, and they built it out of things
that grow: glass seeded and grown, roots that carry what glass carries, rings laid down a
year at a time. The door is the archive's cover and its first page: every hand that ever
pushed on it is in it. Whatever design of the door is chosen, it should be readable as
made (rings, blades, cells, wheels, inscription) and as grown (roots, moss, rings, leaves),
and its light should be theirs.

**Decided: it quietly catalogues.** The archive's makers were to keep every living thing
alive through the cold. Its heart is cooling and it cannot make warmth, so it moves its
collection down the tiers -- living, sleeping, pattern -- and it does the same to whatever
warm thing comes down the chimney. You are entered at the door (the prints), the hunter
is kept living, the sitters are being moved to sleep, the bones are pattern. It is never
shown as an event and nothing is done to anyone on screen. See `claude/ARCHIVE.md`.

## 1. The surface (the first ten minutes)

You are a treasure hunter. You came down a throat of rock glittering with green glass, and
the door rolled shut behind you like a flower closing: a setback, not a tragedy. Below is a
hall built for giants. An enthroned king holds out his hand in blessing. A mural shows tall
hosts welcoming small guests to a feast. Through a cliff-sized window a whole city of lights glitters.
The locals are shy: their lights go out when you come near. A pale swimmer comes to your
lamp and sings. An old hunter camps below, and when you light his cold fire he holds his
hands to it and hums. It is Wonderland: strange rules, kindly meant, and somewhere a way on.

## 2. The truth

**Who built it.** The tall ones, a people of the deep, when the rock was warm; the amber
seams in raw rock are what is left of that warmth. Their craft was a glass that drinks
warmth and gives it back as green-white light, for ages, never as warmth again. Every lamp
they lit made the deep colder. They drained the seams, then the rock, then themselves. The
cold made them slow and pale and took their wanting last. Before it did, they cut the vault.

**What the vault is for.** It is the hearth's chimney, cut up to the warm world, and warmth
comes down it. Its throat is lined with their glass because glass is what they build with;
from above, green-white glass gleaming in the dark looks like gems, and that is the whole
of the "treasure". It is not bait. They knew what warmth would come down, and felt about it
the way you feel about a log.

**Why it seals.** The door is a damper. It opens to warmth arriving and shuts once the
warmth is in, as a flower closes at dusk. Nobody closes it; it runs itself.

**The natives, and why they do not care.** The tall ones are still here, behind windows and
grilles, with everything that lives beside them (the swimmer, the fish, the plant, the
animal), all with the glass grown into them. None of them eats you. All of them take your
warmth, a little, from close by, all the time. They are not cruel: cruelty is warm, and
nothing down here is warm enough for it. You are not a guest or an enemy. You are weather,
the one warm season they get.

**What becomes of hunters who stay.** They cool. Nothing is done to them; heat goes to cold.
It goes in an order: the wanting first (which is why they do not try to leave), then the
hurry, the fear, the moving. It does not hurt; it feels like calm. They settle in the
camp, the warmest nook; later they sit on the window's sill, in the draft, because it
has stopped feeling like anything; later they are bones in the same seat, and nothing takes
any interest in them, because nothing is left to take. The hall shows all four at once:
you (wanting), the camp hunter (tending), the sitters (still), the bones (cold). The player
stands at the start of a line and can see its end. Hunters have come in every age and of
many kinds, as the prints on the door show.

**What this does to things already built (L12).**

| thing | at first | once you know |
|---|---|---|
| windows douse near your lamp | shy people | they save their own light; nobody burns a candle when the sun is in the room |
| the face hums at a sinking stone | a musical statue | the stone is warm from your hand and the glass eyes drink it |
| the plant chatters near you | Alice's talking flowers | warmth makes it tick like a kettle |
| the fire stays lit (D6) | your first kindness | the first time you stoke the hearth; both are true |
| Hold | a lamp or a stone | what you hold takes your warmth, and that is all the city notices. Trade without words is warmth changing hands |

## 3. Telling it without words: the antechamber

Six screens, 3x2; the scaffold in `src/antechamber.c` fits (start top left, water lower
middle and right). All drawable at 320x180, 8px tiles, the 19-colour palette.

```
+------------------+------------------+------------------+
| A  ENTRY         | B  UPPER GALLERY | C  GREAT WINDOW  |
+------------------+------------------+------------------+
| D  UNDERCROFT    | E  FLOOR, BASIN  | F  INNER GATE    |
+------------------+------------------+------------------+
```

**1. The door, from inside.** *A.* Round, a spiral of petals, shut; dust from the lintel for
three seconds; a glint once round the spiral under your lamp; a small green lamp in the
crown. *First:* a flower-lock that knows you; find the key. *Knows:* a damper that shut
because warmth went past it. The crown lamp is a band brighter while you stand near.

**2. The prints (only in the dark).** *A.* With no flame within about eight tiles, faint
green prints come up on the door and the rock round it: dozens, 3-5 px, palms, paws,
three- and six-fingered hands, at many heights. One at your height is a band brighter. Any
flame and they are gone. *First:* a glowing "we were here". *Knows:* every hand that ever
pushed on this door from inside; the stone kept each push's warmth. The bright one is
yours, from before the first frame, and it fades over the first minutes. The green is the
stone's; it is only shaped like you.

**3. Ropes that only go down.** *A, B.* Old lines from the entry ledge and the gallery rail
into the hall: hemp, braided leather, a chain, different knots. Every anchor is at the top.
*First:* earlier hunters rigged the way in. *Knows:* nobody ever rigged a way back up.

**4. The flue slot.** *A.* High in the raw rock over the door, out of reach: a slot lit
inside by one amber seam. Motes sink below it; now and then a dry brown leaf spins down onto
the ledge under it, and they pile there until the reset. *First:* a crack. *Knows:* a flaw the tall
ones did not build, the only thing that reaches the hall from the warm world without being
a hunter. The layer-2 hook (section 4).

**5. The colossus.** *B, E.* A seated figure in profile at the edge of the balcony, two
screens tall, feet in the basin, its back to the chamber beyond and its face to the door. Its forearm reaches down across the hall, palm open toward the
camp, low enough that the camp fire reaches its fingertips; the back of the hand is a
platform. Green glass eyes. *First:* a king's blessing. *Knows:* it is how anyone holds a
hand over a fire. When the fire catches, the fingertips take an amber rim, and below them
the hunter holds his hands to his fire the same way. Everyone in this hall is warming their
hands at something smaller.

**6. The mural, two pictures.** *B.* A band three tiles high along the gallery wall. In the
dark, phosphor: tall figures in procession, with gaps. Under your lamp the phosphor fades
and your amber shows shallow relief in the gaps: small figures with ears and hats.
The panels: small figures come through a petalled door with lights; tall ones meet them,
hands out; the small ones sit in a row, the tall ones round them, hands out; the same row,
worn almost smooth. *First:* a welcome, a feast, a tea party that never ends. *Knows:* they
had no warm pigment, so you are the bare stone between them, shown only by your own light.
It is the hall's use, in order. It never shows a flame.

**7. The far archive.** *B, E, C.* Past the keeper, through the great opening, and through
the window: the rest of the archive (`city.c`), a chamber miles across. A city of lights
on its plain; stepped temples with their tiers lit; domes; an aqueduct; a lantern the size
of a house on a chain from the ceiling; and at its heart a pillar of light going up out of
sight from a tower on the horizon. Another keeper sits out there, a mile off, as ours
does. Now and then a light changes its mind; lanterns rise out of the depths; a procession
carries lights over a bridge. The city *is* this: the tall ones built no streets, only the
keeping. (Not yet: once the camp fire is lit, the pillar a band brighter until the reset.)
*First:* a lost city, and it goes on for ever. *Knows:* it is the archive, it is going dark
from the edges in, and it is still at its cataloguing.

**8. The sitters.** *C.* On the sill, three figures your size face the city in bedrolls,
hats down. Two breathe, a puff of mist a minute, seen only in your light; the nearest head
turns a pixel toward your lamp. The third is bones in the same pose, a bird on its hat.
*First:* sightseers. *Knows:* the draft is strongest here, and they sat in it because it had
stopped feeling like anything. The bones sit as the colossus sits.

**9. The draft.** *All; clearest at D and C.* Dust, smoke and mist drift one way: in low
under the inner gate, across the basin, up past the colossus, out the window; one standing
current in `air.c`. *First:* fresh air. *Knows:* the hall's warmth is being drawn out into
the city through the hearth's mouth.

**10. The camp and its hunter.** *D.* As designed for room 0: a cold fire, a bedroll, a
cairn he tends and adds your stones to; light the fire and he holds his hands to it and
hums. *First:* a friendly eccentric. *Knows:* his lamp is dead and he stopped wanting to
leave long ago. The cairn is heated stones, the oldest way to keep warm, still turned beside
a fire long out. A stone from your hand is warm, and wanting it is his last want: he does to
your stone what the city does to you.

**11. The spilled treasure.** *D.* An old pack split open, green-white chips fanned across
the floor, prised from the vault's throat. Nobody has picked them up. They brighten as your
lamp nears. *First:* loot. *Knows:* what they came for; wanting went first. It is lamp
glass, drinking from you.

**12. The dead lamps.** *D.* In a niche, a row of lamps like yours, set down neatly, glass
dull; at the end, a clean patch in the dust the size of a lamp's foot. Set yours there and
the hunter looks up and hums; nothing else (L6). *First:* spares. *Knows:* each owner set
theirs down when it went out. The tidiness is how slow it was.

**13. The basking trough.** *E.* At the water's edge nearest the camp, a hollow worn
in the paving, polished paler, the swimmer's length (26 px). It is empty while you are in
the room. *First:* a seal's beach. *Knows:* the closest the city comes to the hunters'
fire; something large has lain in it, still, for a very long time.

**14. The swimmer at your lamp.** *E.* The native from room 1. Lamp at the edge: it comes,
holds under it, flank pulsing, surfaces, sings once, stays while the light does. Carry the
lamp off and it does not follow; it waits under where the lamp was, then goes. *First:* a
friendly whale singing to you. *Knows:* it is basking. You are what the lamp is attached to.

**15. Dead seams.** *D, E, where raw rock meets masonry.* Crystal veins run on into the
dressed stone, and there they are black. *First:* crystals, some burnt out. *Knows:* the
city drank these first; where amber turns black is how far it has reached.

**16. The parapet.** *E.* A low stone edge along the opening's foot, the basin on this side
of it, and beyond it the drop into the chamber. Its air comes in over it. *First:* a view.
*Knows:* a hearth's fender. It is not keeping the city from you. It keeps the fire in.

**17. The tall ones.** *E.* On a bridge beyond the parapet, tall still silhouettes. Bring
your lamp to the edge and the near lights of the chamber go out one by one; in the dark, a
murmur; when the lights return, a silhouette is sometimes a step nearer. *First:* shy natives come to see the
newcomer. *Knows:* they put out their own light because yours is enough, and come as close
as the bars allow, as anyone moves toward a stove. They are not looking at you.

## 4. The antechamber as a place

**Why it is shaped like this.** It is a fireplace the size of a cathedral, open on one
side to the city it warmed. The vault passage is the chimney and the door its damper, low
at one end. The opening behind the keeper is the hearth's front, the parapet its fender;
the city's cold air comes in over it, rises past the keeper's back and leaves by the
window, high at the other end. The basin is the cold, pooled along the fender. The keeper
sits with its back to the city and its hand out toward the door, the way you sit at a
fire, waiting for it to be fed. The hunters camp by the door, out of the draft. Never
stated; only true, so the draft, the light and the creatures agree.

**Each screen.**
- **D, the door** (the first frame): the door fills it, you at its foot, the lamp beside
  you; the camp to the right, the keeper's fingers reaching in over it from the right edge.
- **A, the crown**: the door's crown, the raw rock and the hunters' planks up it toward
  the flue's amber.
- **B, the keeper**: its head against the chamber beyond, the pillar of light by its back,
  nothing across it.
- **E, the lap**: the water along the parapet, the city below, the temples, the tall ones on
  their bridge.
- **C, the window**: a round window at the right end, its iris drawn back, the pillar of
  light standing in it; the sitters on its lip. You look through it, never walk out of it.
- **F, the stair**: the tall ones' treads up to the window. The way on is not over the
  parapet in layer 1; it is down through the water.

**What you cannot reach yet.** The flue slot: in the first seconds, one amber point over
the door and a column of sinking motes. Nothing in layer 1 gets you there; the leaves on
the step are its only tell. It is raw rock: the way up, if any, is through something the
tall ones did not make. Never point at it: no pulse, no sound, no framing.

## 5. The two lights

**Amber** is flame: your lamp, the camp fire, the seams of raw rock. It is warmth not yet
spent, the only light that warms and the only one that ends: a hunter's, or old heat the
city has not reached.

**Green-white** is the city's: lamps, glass eyes, flank lights, phosphor, fish. It is warmth
taken and given back with no heat in it; it never goes out and never warms a hand. **A
green lamp is never a person.** It is what a stone keeps after a hand has left it. To the
green, amber is the one thing it cannot make and was made from, and every green thing turns
toward it.

**Where they meet, the flame leans.** Any flame within about five tiles of the city's glass
leans toward it, smoke too, and the glass is a band brighter while it does. Never the
reverse. See it at the door's crown lamp over your lamp on the step, the first thing in the
game for anyone who looks, and at the camp fire. *First:* the fire is curious. *Knows:* it
is being drawn. A behaviour, not a blend: the bakes stay separate, and nothing is drawn in a
colour between them.

## 6. Never show, never explain

- The door open, from either side, in any picture.
- A hunter stopping. The stages are different people; the player joins them up.
- Amber turning green, or green turning amber, on screen.
- A person in a lamp: no faces in glass. Warmth, not souls (ghosts in a lantern are Animal
  Well's).
- A native touching, chasing, eating or harming anyone. They come close. That is all.
- A tall one up close, or whole, in light.
- Gore. Bones are clean, dry and seated.
- Fire in the tall ones' art, or any picture that states the hearth.
- What is above the flue or past the gate; the climbing light; whether the swimmer is a
  person.
- A tally: no set of prints, sitters or lamps to complete (L8), no row of braziers to light.

**For other docs, not changed here.** ROOMS.md's room-0 mural carries a disc, which the
originality constraint rules out: redraw it as a small seated figure in cupped hands. The
camp and the hunter are by the door, at the foot of the room.
