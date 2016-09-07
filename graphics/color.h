/*
 * Copyright (C) 2012-2016 Jacob R. Lifshay
 * This file is part of Voxels.
 *
 * Voxels is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * Voxels is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with Voxels; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 */

#ifndef GRAPHICS_COLOR_H_
#define GRAPHICS_COLOR_H_

#include <cstdint>
#include <type_traits>
#include <limits>
#include "../util/constexpr_assert.h"
#include "../util/interpolate.h"
#include "../util/integer_sequence.h"
#include "../util/constexpr_array.h"

namespace programmerjake
{
namespace voxels
{
namespace graphics
{
template <typename = void>
class SRGBHelper final
{
private:
    static constexpr util::Array<float, std::numeric_limits<std::uint8_t>::max() + 1U>
        linearToSRGBLookupArray = {{
            0.00000000000000000000f,
            0.04984008984501272225f,
            0.08494473023989008289f,
            0.11070205987479768733f,
            0.13180380330527967654f,
            0.15000520429240006566f,
            0.16618571343894924773f,
            0.18085851932400844014f,
            0.19435316156242730690f,
            0.20689573747353274083f,
            0.21864911700176540666f,
            0.22973509755247953302f,
            0.24024750547134202405f,
            0.25026038520259277810f,
            0.25983335153921965846f,
            0.26901521866799851304f,
            0.27784653781682748467f,
            0.28636141889008281627f,
            0.29458886801692404801f,
            0.30255378918686938559f,
            0.31027774743239520696f,
            0.31777955931537318409f,
            0.32507575609404228865f,
            0.33218095151750703743f,
            0.33910813714743261063f,
            0.34586892188950113686f,
            0.35247372806771746005f,
            0.35893195328187724022f,
            0.36525210505647513313f,
            0.37144191365645722301f,
            0.37750842723566293152f,
            0.38345809257743412577f,
            0.38929682400041482803f,
            0.39503006247749556781f,
            0.40066282661051416056f,
            0.40619975678770815351f,
            0.41164515360317828690f,
            0.41700301142169688044f,
            0.42227704781614394081f,
            0.42747072947973107836f,
            0.43258729511421706271f,
            0.43762977571337681848f,
            0.44260101259410241660f,
            0.44750367347263172463f,
            0.45234026683813695863f,
            0.45711315483839606333f,
            0.46182456486104423945f,
            0.46647659996779523848f,
            0.47107124831710284923f,
            0.47561039169225744587f,
            0.48009581323628059418f,
            0.48452920448170694809f,
            0.48891217175203221302f,
            0.49324624200193639508f,
            0.49753286815509849294f,
            0.50177343399128451195f,
            0.50596925862823522821f,
            0.51012160063855419535f,
            0.51423166183717574565f,
            0.51830059077097331390f,
            0.52232948593856322681f,
            0.52631939876529492070f,
            0.53027133635573359542f,
            0.53418626404358331320f,
            0.53806510775692315326f,
            0.54190875621479849616f,
            0.54571806296959163785f,
            0.54949384830816317837f,
            0.55323690102348437624f,
            0.55694798006735060191f,
            0.56062781609375966834f,
            0.56427711290164105451f,
            0.56789654878481979072f,
            0.57148677779638070006f,
            0.57504843093395492856f,
            0.57858211725187265895f,
            0.58208842490560609647f,
            0.58556792213345868656f,
            0.58902115818003433264f,
            0.59244866416563909257f,
            0.59585095390542301333f,
            0.59922852468175751847f,
            0.60258185797306064924f,
            0.60591142014202544128f,
            0.60921766308597310315f,
            0.61250102485184006765f,
            0.61576193021811429725f,
            0.61900079124585955790f,
            0.62221800780080505951f,
            0.62541396804833040175f,
            0.62858904892304082977f,
            0.63174361657450420437f,
            0.63487802679060775949f,
            0.63799262539988870157f,
            0.64108774865409714329f,
            0.64416372359216198849f,
            0.64722086838664950353f,
            0.65025949267372979898f,
            0.65327989786759774290f,
            0.65628237746023142505f,
            0.65926721730731273256f,
            0.66223469590108046686f,
            0.66518508463083635385f,
            0.66811864803177793474f,
            0.67103564402278936408f,
            0.67393632413378130532f,
            0.67682093372313414686f,
            0.67968971218576443251f,
            0.68254289315230249696f,
            0.68538070467983962862f,
            0.68820336943467547241f,
            0.69101110486747067345f,
            0.69380412338118580389f,
            0.69658263249216527374f,
            0.69934683498470408131f,
            0.70209692905941579648f,
            0.70483310847570198816f,
            0.70755556268860631075f,
            0.71026447698032056757f,
            0.71296003258659519045f,
            0.71564240681829264309f,
            0.71831177317830920145f,
            0.72096830147407832695f,
            0.72361215792585736948f,
            0.72624350527098856451f,
            0.72886250286431517322f,
            0.73146930677492411160f,
            0.73406406987937748168f,
            0.73664694195158701828f,
            0.73921806974947756068f,
            0.74177759709857821848f,
            0.74432566497267289419f,
            0.74686241157163522258f,
            0.74938797239656676302f,
            0.75190248032235141156f,
            0.75440606566773346114f,
            0.75689885626302151049f,
            0.75938097751551548566f,
            0.76185255247274937372f,
            0.76431370188363785984f,
            0.76676454425761089068f,
            0.76920519592181624390f,
            0.77163577107646645211f,
            0.77405638184840289731f,
            0.77646713834294654588f,
            0.77886814869410162506f,
            0.78125951911317553673f,
            0.78364135393587545675f,
            0.78601375566793936525f,
            0.78837682502935669042f,
            0.79073066099723131417f,
            0.79307536084733737788f,
            0.79541102019441613127f,
            0.79773773303125998205f,
            0.80005559176662792145f,
            0.80236468726203461600f,
            0.80466510886745366287f,
            0.80695694445597380023f,
            0.80924028045744524051f,
            0.81151520189115174841f,
            0.81378179239754261347f,
            0.81604013426905726417f,
            0.81829030848007393407f,
            0.82053239471601251614f,
            0.82276647140162052651f,
            0.82499261572846993983f,
            0.82721090368169155284f,
            0.82942141006597247715f,
            0.83162420853084135491f,
            0.83381937159526492904f,
            0.83600697067157868033f,
            0.83818707608877336608f,
            0.84035975711515845546f,
            0.84252508198042265451f,
            0.84468311789711094681f,
            0.84683393108153684177f,
            0.84897758677414782053f,
            0.85111414925936129793f,
            0.85324368188488777530f,
            0.85536624708055724388f,
            0.85748190637666430819f,
            0.85959072042184693436f,
            0.86169274900051318693f,
            0.86378805104982979887f,
            0.86587668467628592235f,
            0.86795870717184493097f,
            0.87003417502969668692f,
            0.87210314395962224755f,
            0.87416566890298256546f,
            0.87622180404734233166f,
            0.87827160284073972392f,
            0.88031511800561245005f,
            0.88235240155239011817f,
            0.88438350479276262266f,
            0.88640847835263390439f,
            0.88842737218477012688f,
            0.89044023558115100502f,
            0.89244711718503273017f,
            0.89444806500273065349f,
            0.89644312641512961858f,
            0.89843234818892957368f,
            0.90041577648763384293f,
            0.90239345688228719478f,
            0.90436543436197061350f,
            0.90633175334405945589f,
            0.90829245768425146007f,
            0.91024759068637086578f,
            0.91219719511195470599f,
            0.91414131318962713700f,
            0.91607998662426748881f,
            0.91801325660597753893f,
            0.91994116381885334041f,
            0.92186374844956676865f,
            0.92378105019576179181f,
            0.92569310827427031453f,
            0.92759996142915229641f,
            0.92950164793956470217f,
            0.93139820562746370241f,
            0.93328967186514440947f,
            0.93517608358262230400f,
            0.93705747727486038299f,
            0.93893388900884593939f,
            0.94080535443052076747f,
            0.94267190877156847546f,
            0.94453358685606247831f,
            0.94639042310697813913f,
            0.94824245155257242567f,
            0.95008970583263435111f,
            0.95193221920460937307f,
            0.95377002454960083365f,
            0.95560315437825143455f,
            0.95743164083650765589f,
            0.95925551571126994440f,
            0.96107481043593141673f,
            0.96288955609580774606f,
            0.96469978343346082515f,
            0.96650552285391872656f,
            0.96830680442979441053f,
            0.97010365790630556273f,
            0.97189611270619787881f,
            0.97368419793457404838f,
            0.97546794238363062973f,
            0.97724737453730494660f,
            0.97902252257583408052f,
            0.98079341438022797601f,
            0.98256007753665862170f,
            0.98432253934076721763f,
            0.98608082680189118800f,
            0.98783496664721284908f,
            0.98958498532583149413f,
            0.99133090901276061043f,
            0.99307276361285189868f,
            0.99481057476464772103f,
            0.99654436784416356175f,
            0.99827416796860204319f,
            1.00000000000000000000f,
        }};
    static constexpr util::Array<float, std::numeric_limits<std::uint8_t>::max() + 1U>
        srgbToLinearLookupArray = {{
            0.00000000000000000000f,
            0.00030352698354883749f,
            0.00060705396709767498f,
            0.00091058095064651247f,
            0.00121410793419534996f,
            0.00151763491774418745f,
            0.00182116190129302494f,
            0.00212468888484186244f,
            0.00242821586839069993f,
            0.00273174285193953742f,
            0.00303526983548837491f,
            0.00334653576389915849f,
            0.00367650732404743494f,
            0.00402471701849630428f,
            0.00439144203741029283f,
            0.00477695348069372709f,
            0.00518151670233838514f,
            0.00560539162420272078f,
            0.00604883302285705259f,
            0.00651209079259447151f,
            0.00699541018726538556f,
            0.00749903204322617003f,
            0.00802319298538499338f,
            0.00856812561806930196f,
            0.00913405870222078614f,
            0.00972121732023784360f,
            0.01032982302962693702f,
            0.01096009400648824092f,
            0.01161224517974388144f,
            0.01228648835691586705f,
            0.01298303234217300799f,
            0.01370208304728968318f,
            0.01444384359609254090f,
            0.01520851442291270550f,
            0.01599629336550962806f,
            0.01680737575288737653f,
            0.01764195448838407693f,
            0.01850022012837968846f,
            0.01938236095693572256f,
            0.02028856305665239095f,
            0.02121901037600355516f,
            0.02217388479338737574f,
            0.02315336617811040449f,
            0.02415763244850474842f,
            0.02518685962736162244f,
            0.02624122189484989029f,
            0.02732089163907488939f,
            0.02842603950442078827f,
            0.02955683443780879705f,
            0.03071344373299362175f,
            0.03189603307301151595f,
            0.03310476657088504597f,
            0.03433980680868216566f,
            0.03560131487502032176f,
            0.03688945040110001638f,
            0.03820437159534648287f,
            0.03954623527673283348f,
            0.04091519690685316839f,
            0.04231141062080965386f,
            0.04373502925697344809f,
            0.04518620438567554375f,
            0.04666508633688007742f,
            0.04817182422688940327f,
            0.04970656598412721641f,
            0.05126945837404322142f,
            0.05286064702318025315f,
            0.05448027644244235380f,
            0.05612849004960007690f,
            0.05780543019106721120f,
            0.05951123816298118289f,
            0.06124605423161759166f,
            0.06301001765316765442f,
            0.06480326669290576055f,
            0.06662593864377287549f,
            0.06847816984440015791f,
            0.07036009569659587177f,
            0.07227185068231747238f,
            0.07421356838014961861f,
            0.07618538148130780625f,
            0.07818742180518632529f,
            0.08021982031446831214f,
            0.08228270712981479128f,
            0.08437621154414877707f,
            0.08650046203654973083f,
            0.08865558628577293744f,
            0.09084171118340767766f,
            0.09305896284668742270f,
            0.09530746663096466515f,
            0.09758734714186242176f,
            0.09989872824711389728f,
            0.10224173308810128179f,
            0.10461648409110416539f,
            0.10702310297826759212f,
            0.10946171077829933663f,
            0.11193242783690557309f,
            0.11443537382697371221f,
            0.11697066775851080990f,
            0.11953842798834559733f,
            0.12213877222960184632f,
            0.12477181756095046558f,
            0.12743768043564742000f,
            0.13013647669036427752f,
            0.13286832155381791428f,
            0.13563332965520564813f,
            0.13843161503245182258f,
            0.14126329114027162737f,
            0.14412847085805771652f,
            0.14702726649759497067f,
            0.14995978981060854572f,
            0.15292615199615015532f,
            0.15592646370782734796f,
            0.15896083506088036265f,
            0.16202937563911097682f,
            0.16513219450166759853f,
            0.16826940018969070004f,
            0.17144110073282254177f,
            0.17464740365558499405f,
            0.17788841598362912892f,
            0.18116424424986012407f,
            0.18447499450044089748f,
            0.18782077230067777215f,
            0.19120168274079135650f,
            0.19461783044157571707f,
            0.19806931955994881531f,
            0.20155625379439708023f,
            0.20507873639031689228f,
            0.20863687014525566168f,
            0.21223075741405509558f,
            0.21586050011389916376f,
            0.21952619972926919044f,
            0.22322795731680842173f,
            0.22696587351009834244f,
            0.23074004852434894389f,
            0.23455058216100507424f,
            0.23839757381227093628f,
            0.24228112246555473278f,
            0.24620132670783539773f,
            0.25015828472995329186f,
            0.25415209433082668356f,
            0.25818285292159578078f,
            0.26225065752969602631f,
            0.26635560480286231732f,
            0.27049779101306576071f,
            0.27467731206038452786f,
            0.27889426347681032661f,
            0.28314874042999196365f,
            0.28744083772691742804f,
            0.29177064981753588481f,
            0.29613827079832092823f,
            0.30054379441577640546f,
            0.30498731406988608436f,
            0.30946892281750840332f,
            0.31398871337571750630f,
            0.31854677812509173271f,
            0.32314320911295069961f,
            0.32777809805654208227f,
            0.33245153634617916890f,
            0.33716361504833023627f,
            0.34191442490866076452f,
            0.34670405635502948223f,
            0.35153259950043920621f,
            0.35640014414594341519f,
            0.36130677978350947128f,
            0.36625259559883937974f,
            0.37123768047414895373f,
            0.37626212299090622879f,
            0.38132601143252994946f,
            0.38642943378704892998f,
            0.39157247774972306984f,
            0.39675523072562678581f,
            0.40197777983219560243f,
            0.40724021190173662439f,
            0.41254261348390359646f,
            0.41788507084813723860f,
            0.42326766998607152747f,
            0.42869049661390657865f,
            0.43415363617474876807f,
            0.43965717384091871565f,
            0.44520119451622773904f,
            0.45078578283822337076f,
            0.45641102318040451785f,
            0.46207699965440682927f,
            0.46778379611215882319f,
            0.47353149614800931285f,
            0.47932018310082665748f,
            0.48514994005607035231f,
            0.49102084984783545968f,
            0.49693299506087037193f,
            0.50288645803256838517f,
            0.50888132085493355242f,
            0.51491766537652127368f,
            0.52099557320435407020f,
            0.52711512570581298033f,
            0.53327640401050500431f,
            0.53947948901210701590f,
            0.54572446137018654955f,
            0.55201140151199986267f,
            0.55834038963426766390f,
            0.56471150570492888961f,
            0.57112482946487290262f,
            0.57758044042965047905f,
            0.58407841789116394118f,
            0.59061884091933678669f,
            0.59720178836376315708f,
            0.60382733885533748072f,
            0.61049557080786461909f,
            0.61720656241965083749f,
            0.62396039167507591522f,
            0.63075713634614670319f,
            0.63759687399403243096f,
            0.64447968197058205847f,
            0.65140563741982396216f,
            0.65837481727944823886f,
            0.66538729828227190519f,
            0.67244315695768726455f,
            0.67954246963309370826f,
            0.68668531243531321210f,
            0.69387176129198978414f,
            0.70110189193297311475f,
            0.70837577989168667465f,
            0.71569350050648050199f,
            0.72305512892196891466f,
            0.73046074009035337951f,
            0.73791040877273076549f,
            0.74540420954038720338f,
            0.75294221677607777049f,
            0.76052450467529221440f,
            0.76815114724750692575f,
            0.77582221831742336616f,
            0.78353779152619315321f,
            0.79129794033263000078f,
            0.79910273801440870930f,
            0.80695225766925139651f,
            0.81484657221610115625f,
            0.82278575439628332882f,
            0.83076987677465456326f,
            0.83879901174073984868f,
            0.84687323150985768814f,
            0.85499260812423358582f,
            0.86315721345410201471f,
            0.87136711919879702932f,
            0.87962239688783168466f,
            0.88792311788196642003f,
            0.89626935337426656313f,
            0.90466117439114910745f,
            0.91309865179341891275f,
            0.92158185627729447635f,
            0.93011085837542341972f,
            0.93868572845788783278f,
            0.94730653673319961564f,
            0.95597335324928595486f,
            0.96468624789446506936f,
            0.97344529039841235833f,
            0.98225055033311708144f,
            0.99110209711382969930f,
            1.00000000000000000000f,
        }};
    static_assert(linearToSRGBLookupArray.size() == 0x100, "");
    static_assert(srgbToLinearLookupArray.size() == 0x100, "");

    static constexpr float srgbToLinearHelper(float scaledV, unsigned minIndex) noexcept
    {
        return util::interpolate(scaledV - minIndex,
                                 srgbToLinearLookupArray[minIndex],
                                 minIndex + 1 >= srgbToLinearLookupArray.size() ?
                                     1 :
                                     srgbToLinearLookupArray[minIndex + 1]);
    }
    static constexpr float srgbToLinearHelper(float scaledV) noexcept
    {
        return srgbToLinearHelper(scaledV, (unsigned)scaledV);
    }
    static constexpr float linearToSRGBHelper(float scaledV, unsigned minIndex) noexcept
    {
        return util::interpolate(scaledV - minIndex,
                                 linearToSRGBLookupArray[minIndex],
                                 minIndex + 1 >= linearToSRGBLookupArray.size() ?
                                     1 :
                                     linearToSRGBLookupArray[minIndex + 1]);
    }
    static constexpr float linearToSRGBHelper(float scaledV) noexcept
    {
        return linearToSRGBHelper(scaledV, (unsigned)scaledV);
    }

public:
    static constexpr float srgbToLinear(float v) noexcept
    {
        return (constexprAssert(v >= 0 && v <= 1), srgbToLinearHelper(v * 255));
    }
    static constexpr float linearToSRGB(float v) noexcept
    {
        return (constexprAssert(v >= 0 && v <= 1), linearToSRGBHelper(v * 255));
    }
};

template <typename T>
constexpr util::Array<float, std::numeric_limits<std::uint8_t>::max() + 1U>
    SRGBHelper<T>::srgbToLinearLookupArray;

template <typename T>
constexpr util::Array<float, std::numeric_limits<std::uint8_t>::max() + 1U>
    SRGBHelper<T>::linearToSRGBLookupArray;

constexpr float srgbToLinear(float v) noexcept
{
    return SRGBHelper<>::srgbToLinear(v);
}

constexpr float linearToSRGB(float v) noexcept
{
    return SRGBHelper<>::linearToSRGB(v);
}

template <typename T,
          bool isUnsignedInteger = (std::is_arithmetic<T>::value && std::is_unsigned<T>::value
                                    && (std::numeric_limits<T>::digits < 64)),
          bool isFloat = std::is_floating_point<T>::value>
struct ColorTraitsImplementation final
{
    static_assert(isUnsignedInteger || isFloat, "invalid type T");
    static constexpr T max = isFloat ? 1 : std::numeric_limits<T>::max();
    static constexpr std::size_t digits = std::numeric_limits<T>::digits;
    typedef typename std::
        conditional<isFloat,
                    T,
                    typename std::conditional<digits <= 8,
                                              std::uint16_t,
                                              typename std::conditional<digits <= 16,
                                                                        std::uint32_t,
                                                                        std::uint64_t>::type>::
                        type>::type WideType;
};

template <typename T>
struct ColorTraits final
{
    static constexpr T min = 0;
    static constexpr T max = ColorTraitsImplementation<T>::max;

private:
    template <typename T2,
              typename = typename std::enable_if<!std::is_floating_point<T>::value
                                                 && !std::is_floating_point<T2>::value>::type>
    static constexpr T2 castToImplementation(T v, T2) noexcept
    {
        typedef ColorTraits<T2> TargetTraits;
        typedef
            typename std::common_type<WideType, typename TargetTraits::WideType>::type CommonType;
        return static_cast<T2>(static_cast<CommonType>(v) * TargetTraits::max / max);
    }
    template <typename T2,
              typename = typename std::enable_if<std::is_floating_point<T2>::value>::type>
    static constexpr T2 castToImplementation(T v, T2, int = 0) noexcept
    {
        typedef typename std::common_type<T, T2>::type CommonType;
        typedef ColorTraits<T2> TargetTraits;
        return static_cast<T2>(
            static_cast<CommonType>(v)
            * static_cast<CommonType>(TargetTraits::max / static_cast<long double>(max)));
    }
    template <typename T2,
              typename = typename std::enable_if<std::is_floating_point<T>::value
                                                 && !std::is_floating_point<T2>::value>::type>
    static constexpr T2 castToImplementation(T v, T2, long = 0) noexcept
    {
        typedef typename std::common_type<T, T2>::type CommonType;
        typedef ColorTraits<T2> TargetTraits;
        return static_cast<T2>(
            0.5f
            + static_cast<CommonType>(v)
                  * static_cast<CommonType>(TargetTraits::max / static_cast<long double>(max)));
    }
    static constexpr T castToImplementation(T v, T) noexcept
    {
        return v;
    }

public:
    template <typename T2>
    static constexpr T2 castTo(T v) noexcept
    {
        return castToImplementation(v, T2());
    }
    static constexpr T limit(T v) noexcept
    {
        return (constexprAssert(v >= min && v <= max), v);
    }
    static constexpr T multiply(T a, T b) noexcept
    {
        return static_cast<WideType>(a) * b;
    }
    typedef typename ColorTraitsImplementation<T>::WideType WideType;
};

template <typename T>
struct BasicColor final
{
    typedef T ValueType;
    T red;
    T green;
    T blue;
    T opacity;
    constexpr BasicColor() : red(0), green(0), blue(0), opacity(0)
    {
    }
    constexpr explicit BasicColor(T value, T opacity = ColorTraits<T>::max)
        : red(ColorTraits<T>::limit(value)),
          green(ColorTraits<T>::limit(value)),
          blue(ColorTraits<T>::limit(value)),
          opacity(ColorTraits<T>::limit(opacity))
    {
    }
    template <typename T2>
    constexpr explicit BasicColor(BasicColor<T2> v)
        : red(ColorTraits<T2>::template castTo<T>(v.red)),
          green(ColorTraits<T2>::template castTo<T>(v.green)),
          blue(ColorTraits<T2>::template castTo<T>(v.blue)),
          opacity(ColorTraits<T2>::template castTo<T>(v.opacity))
    {
    }
    constexpr BasicColor(BasicColor value, T opacity)
        : red(value.red),
          green(value.green),
          blue(value.blue),
          opacity(ColorTraits<T>::multiply(value.opacity, ColorTraits<T>::limit(opacity)))
    {
    }
    constexpr BasicColor(T red, T green, T blue, T opacity = ColorTraits<T>::max)
        : red(ColorTraits<T>::limit(red)),
          green(ColorTraits<T>::limit(green)),
          blue(ColorTraits<T>::limit(blue)),
          opacity(ColorTraits<T>::limit(opacity))
    {
    }
    friend constexpr BasicColor operator*(BasicColor a, BasicColor b)
    {
        return BasicColor(ColorTraits<T>::multiply(a.red, b.red),
                          ColorTraits<T>::multiply(a.green, b.green),
                          ColorTraits<T>::multiply(a.blue, b.blue),
                          ColorTraits<T>::multiply(a.opacity, b.opacity));
    }
    BasicColor &operator*=(BasicColor rt)
    {
        *this = *this *rt;
        return *this;
    }
    constexpr bool operator==(BasicColor rt) const
    {
        return red == rt.red && green == rt.green && blue == rt.blue && opacity == rt.opacity;
    }
    constexpr bool operator!=(BasicColor rt) const
    {
        return !operator==(rt);
    }
};

template <typename T>
constexpr BasicColor<T> colorize(BasicColor<T> a, BasicColor<T> b)
{
    return a * b;
}

typedef BasicColor<std::uint8_t> ColorU8;
typedef BasicColor<std::uint16_t> ColorU16;
typedef BasicColor<float> ColorF;

constexpr ColorU8 colorize(ColorF a, ColorU8 b)
{
    return ColorU8(colorize(a, ColorF(b)));
}

constexpr ColorU16 colorize(ColorF a, ColorU16 b)
{
    return ColorU16(colorize(a, ColorF(b)));
}

template <typename T>
constexpr BasicColor<T> grayscaleA(
    typename std::enable_if<true, T>::type value,
    typename std::enable_if<true, T>::type opacity) // use enable_if to prevent type deduction
{
    return BasicColor<T>(value, opacity);
}

constexpr ColorU8 grayscaleAU8(std::uint8_t value, std::uint8_t opacity)
{
    return grayscaleA<std::uint8_t>(value, opacity);
}

constexpr ColorU16 grayscaleAU16(std::uint8_t value, std::uint8_t opacity)
{
    return grayscaleA<std::uint16_t>(value, opacity);
}

constexpr ColorF grayscaleAF(float value, float opacity)
{
    return grayscaleA<float>(value, opacity);
}

template <typename T>
constexpr BasicColor<T> grayscale(
    typename std::enable_if<true, T>::type value) // use enable_if to prevent type deduction
{
    return BasicColor<T>(value);
}

constexpr ColorU8 grayscaleU8(std::uint8_t value)
{
    return grayscale<std::uint8_t>(value);
}

constexpr ColorU16 grayscaleU16(std::uint16_t value)
{
    return grayscale<std::uint16_t>(value);
}

constexpr ColorF grayscaleF(float value)
{
    return grayscale<float>(value);
}

template <typename T>
constexpr BasicColor<T> rgb(
    typename std::enable_if<true, T>::type red,
    typename std::enable_if<true, T>::type green,
    typename std::enable_if<true, T>::type blue) // use enable_if to prevent type deduction
{
    return BasicColor<T>(red, green, blue);
}

constexpr ColorU8 rgbU8(std::uint8_t red, std::uint8_t green, std::uint8_t blue)
{
    return rgb<std::uint8_t>(red, green, blue);
}

constexpr ColorU16 rgbU16(std::uint16_t red, std::uint16_t green, std::uint16_t blue)
{
    return rgb<std::uint16_t>(red, green, blue);
}

constexpr ColorF rgbF(float red, float green, float blue)
{
    return rgb<float>(red, green, blue);
}

template <typename T>
constexpr BasicColor<T> rgba(
    typename std::enable_if<true, T>::type red,
    typename std::enable_if<true, T>::type green,
    typename std::enable_if<true, T>::type blue,
    typename std::enable_if<true, T>::type opacity) // use enable_if to prevent type deduction
{
    return BasicColor<T>(red, green, blue, opacity);
}

constexpr ColorU8 rgbaU8(std::uint8_t red,
                         std::uint8_t green,
                         std::uint8_t blue,
                         std::uint8_t opacity)
{
    return rgba<std::uint8_t>(red, green, blue, opacity);
}

constexpr ColorU16 rgbaU16(std::uint16_t red,
                           std::uint16_t green,
                           std::uint16_t blue,
                           std::uint16_t opacity)
{
    return rgba<std::uint16_t>(red, green, blue, opacity);
}

constexpr ColorF rgbaF(float red, float green, float blue, float opacity)
{
    return rgba<float>(red, green, blue, opacity);
}

template <typename T>
constexpr BasicColor<T> colorizeIdentity()
{
    return BasicColor<T>(
        ColorTraits<T>::max, ColorTraits<T>::max, ColorTraits<T>::max, ColorTraits<T>::max);
}

constexpr ColorU8 colorizeIdentityU8()
{
    return colorizeIdentity<std::uint8_t>();
}

constexpr ColorU16 colorizeIdentityU16()
{
    return colorizeIdentity<std::uint16_t>();
}

constexpr ColorF colorizeIdentityF()
{
    return colorizeIdentity<float>();
}

constexpr ColorF interpolate(float v, ColorF a, ColorF b)
{
    using util::interpolate;
    return ColorF(interpolate(v, a.red, b.red),
                  interpolate(v, a.green, b.green),
                  interpolate(v, a.blue, b.blue),
                  interpolate(v, a.opacity, b.opacity));
}

constexpr ColorU8 interpolate(float v, ColorU8 a, ColorU8 b)
{
    return ColorU8(interpolate(v, ColorF(a), ColorF(b)));
}

constexpr ColorU16 interpolate(float v, ColorU16 a, ColorU16 b)
{
    return ColorU16(interpolate(v, ColorF(a), ColorF(b)));
}

constexpr ColorF linearToSRGB(ColorF v) noexcept
{
    return rgbaF(linearToSRGB(v.red), linearToSRGB(v.green), linearToSRGB(v.blue), v.opacity);
}

constexpr ColorF srgbToLinear(ColorF v) noexcept
{
    return rgbaF(srgbToLinear(v.red), srgbToLinear(v.green), srgbToLinear(v.blue), v.opacity);
}
}
}
}

#endif /* GRAPHICS_COLOR_H_ */
