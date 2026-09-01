# Physical specification
This document describes the requirements imposed by the OpenPrintTag standard on the physical tag itself, its mechanical and electrical properties. Currently, the **OpenPrintTag standard supports only the reference `OpenPrintTag MK1` tag**. A more generic specification, allowing deviations from the exact reference design, **will be released later**.

## Reader specification
To comply with the OpenPrintTag standard, a NFC reader SHALL meet the following parameters. Meeting the parameters is necessary to ensure readability for various tag attachment positions and spool sizes supported by the standard.
<table>
<tr>
<th>Parameter</th>
<th>Min</th>
<th>Typical</th>
<th>Max</th>
<th>Unit</th>
</tr>
<tr>
<tr>
<td>Communication protocol</td>
<td colspan="4">ISO/IEC 15693-3 (NFC-V)</td>
</tr>
<tr>
<td>Antenna shape</td>
<td colspan="4">Circular</td>
</tr>
<tr>
<td>Antenna diameter (Ø)</td>
<td align="right">72</td>
<td align="right">75</td>
<td align="right">80</td>
<td>mm</td>
</tr>
<tr>
<td>RF output power</td>
<td align="right"></td>
<td align="right">1</td>
<td align="right">1.6</td>
<td>W</td>
</tr>
<tr>
<td>Resonant frequency</td>
<td align="right">13.553</td>
<td align="right">13.56</td>
<td align="right">13.567</td>
<td>MHz</td>
</tr>
</table>

1. The reader antenna SHALL be installed to be parallel with the side of the mounted spool.
1. The reader antenna SHALL be installed to be concentric with the mounted spool with maximum 5 cm of allowed eccentricity.
    1. _Note: If the spools are mounted using a pole going through the spool center hole, there will be eccentricity introduced by the difference of the rod and the spool hole diameters._
    1. _Note: If the spools are laid on rollers, there will be eccentricity introduced by different outer diameters of the spools._
1. The reader antenna SHOULD be installed to be as close to the filament spool as possible.
1. The reader antenna SHOULD be installed in a sufficient distance from components (such as large metal parts or NFC tags not intended to be read by the reader) that can drain energy from the magnetic field.
    1. _Note: This needs to be experimentally verified for every installation scenario._

<img src="media/antenna_placement.svg" style="max-width: 512px;">

## Reference `OpenPrintTag MK1` tag
`OpenPrintTag MK1` is a passive paper-label NFC tag with an embedded antenna and NFC integrated circuit, compliant with the ISO/IEC 15693 vicinity-RFID standard (13.56 MHz).

<a href="media/tag_mk1.pdf">Technical drawing</a>

## Placement guidelines on filament spools
1. The tag SHALL be attached to a flat, non-metallic surface.
1. The tag SHOULD be placed at either flange of the spool (e.g. not in the middle of the barrel) to ensure compatibility with generic readers.
1. The tag SHALL be placed no further than 130 mm from either side of the spool.
    1. *Note: This limitation ensures the tag remains readable from both sides of the spool.*
1. The tag SHALL be positioned coaxially with the central axis of the spool.
1. The design of the material container SHALL provide sufficient mechanical protection of the tag's integrated circuit (IC) during the entire lifecycle.
1. To achieve this, the tag SHOULD be placed inside a debossed pocket or cavity with a minimum depth of 0.25 mm. The cavity MAY be covered to fully enclose the tag.


<img src="media/tag_placement.svg" style="max-width: 512px;">


## Mechanical parameters

<table>
<tr>
<th>Parameter</th>
<th>Min</th>
<th>Typical</th>
<th>Max</th>
<th>Unit</th>
</tr>
<tr>
<td>Shape</td>
<td colspan="4">Circular</td>
</tr>
<tr>
<td>Outer diameter (Ø)</td>
<td align="right">89.5</td>
<td align="right">90</td>
<td align="right">90.5</td>
<td>mm</td>
</tr>
<tr>
<td>Inner diameter (Ø)</td>
<td align="right">63.5</td>
<td align="right">64</td>
<td align="right">64.5</td>
<td>mm</td>
</tr>
<tr>
<td>Antenna outer diameter (Ø)</td>
<td align="right">78.6</td>
<td align="right">79.1</td>
<td align="right">79.6</td>
<td>mm</td>
</tr>
<tr>
<td>Antenna inner diameter (Ø)</td>
<td align="right">73.5</td>
<td align="right">74</td>
<td align="right">74.5</td>
<td>mm</td>
</tr>
<tr>
<td>Top material</td>
<td colspan="4">Paper 80 g/m²</td>
</tr>
<tr>
<td>Top thickness</td>
<td align="right">90</td>
<td align="right">100</td>
<td align="right">110</td>
<td>μm</td>
</tr>
<tr>
<td>Winding material</td>
<td colspan="4">Aluminum foil</td>
</tr>
<tr>
<td>Winding thickness</td>
<td align="right">25</td>
<td align="right">30</td>
<td align="right">35</td>
<td>μm</td>
</tr>
<tr>
<td>Bottom material</td>
<td colspan="4">PET</td>
</tr>
<tr>
<td>Bottom thickness</td>
<td align="right">45</td>
<td align="right">50</td>
<td align="right">55</td>
<td>μm</td>
</tr>
<tr>
<td>Adhesive material</td>
<td colspan="4"><a href="media/adhesive_spec.pdf">Hot Melt Adhesive</a></td>
<tr>
</tr>
<td>Adhesive thickness</td>
<td align="right">15</td>
<td align="right">20</td>
<td align="right">25</td>
<td>μm</td>
</tr>
<tr>
<td>Total thickness</td>
<td align="right">0.14</td>
<td align="right">0.15</td>
<td align="right">0.16</td>
<td>mm</td>
</tr>
</table>

### Electrical parameters
<table>
<tr>
<th>Parameter</th>
<th>Min</th>
<th>Typical</th>
<th>Max</th>
<th>Unit</th>
</tr>
<tr>
<td>Communication protocol</td>
<td colspan="4">ISO/IEC 15693 (NFC-V)</td>
</tr>
<tr>
<td>Integrated Circuit</td>
<td colspan="4"><a href="https://www.nxp.com/docs/en/data-sheet/SL2S2602.pdf" target="_blank">NXP ICODE SLX2</a></td>
</tr>
<td>Communication speed</td>
<td></td>
<td align="right">53</td>
<td></td>
<td>kb/s</td>
</tr>
<tr>
<td>Memory size</td>
<td></td>
<td align="right">316</td>
<td></td>
<td>Byte</td>
</tr>
<tr>
<td>Write endurance</td>
<td></td>
<td align="right">100,000</td>
<td></td>
<td>cycles</td>
</tr>
<tr>
<td>Retention time</td>
<td align="right">10</td>
<td></td>
<td></td>
<td>years</td>
</tr>
<tr>
<td>Operating temperature</td>
<td align="right">+15</td>
<td align="right">+23</td>
<td align="right">+45</td>
<td>°C</td>
</tr>
<tr>
<td>Storage temperature</td>
<td align="right">−55</td>
<td align="right">+23</td>
<td align="right">+80</td>
<td>°C</td>
</tr>
<tr>
<td>Operating frequency</td>
<td align="right">13.553</td>
<td align="right">13.56</td>
<td align="right">13.567</td>
<td>MHz</td>
</tr>
<tr>
<td>Antenna type</td>
<td colspan="4">Printed coil</td>
</tr>
<tr>
<td>Number of coil turns</td>
<td></td>
<td align="right">5</td>
<td></td>
<td></td>
</tr>
<tr>
<td>Antenna inductance at 13.56 MHz</td>
<td align="right">TBD</td>
<td align="right">6.91</td>
<td align="right">TBD</td>
<td>μH</td>
</tr>
<tr>
<td>Antenna DC resistance</td>
<td align="right">TBD</td>
<td align="right">4.565</td>
<td align="right">TBD</td>
<td>Ohm</td>
</tr>
<tr>
<td>Minimum operating voltage on unloaded antenna</td>
<td align="right">1</td>
<td align="right">1.1</td>
<td align="right">1.3</td>
<td>V_rms</td>
</tr>
<tr>
<td>Minimum magnetic field strength</td>
<td align="right">12.376</td>
<td align="right">14.8</td>
<td></td>
<td>mA/m_rms</td>
</tr>
</table>
