#include <groth16/groth16.hpp>
/*
old mcl code:
void precompute_groth16_values(
    const Groth16VerifierKeyInput *vk,
    Groth16VerifierKeyPrecomputedValues *vkPrecomputed)
{
    // pre-compute e(α, β)
    mclBn_pairing(&vkPrecomputed->eAlphaBeta, &vk->alpha, &vk->beta);

    // pre-compute -[δ]₂
    mclBnG2_neg(&vkPrecomputed->deltaNeg, &vk->delta);

    // pre-compute -[γ]₂
    mclBnG2_neg(&vkPrecomputed->gammaNeg, &vk->gamma);
}

int verify_groth16_proof_precomputed(
    const Groth16VerifierKeyInput *vk,
    const Groth16VerifierKeyPrecomputedValues *vkPrecomputed,
    const Groth16ProofInput *proof,
    const mclBnFr *publicInputs)
{
    // [Σᵥ (Kᵥ₊₁ * publicInputs[v])]₁
    mclBnG1 sumKTimesPub = vk->k[0];

    // value to store Kᵥ₊₁ * publicInputs[v]
    mclBnG1 tmpKvTimesPubv;

    // compute K₁ * publicInputs[0]
    mclBnG1_mul(&tmpKvTimesPubv, &vk->k[1], &publicInputs[0]);
    // sumKTimesPub += K₁ * publicInputs[0]
    mclBnG1_add(&sumKTimesPub, &sumKTimesPub, &tmpKvTimesPubv);

    // compute K₂ * publicInputs[1]
    mclBnG1_mul(&tmpKvTimesPubv, &vk->k[2], &publicInputs[1]);
    // sumKTimesPub += K₂ * publicInputs[1]
    mclBnG1_add(&sumKTimesPub, &sumKTimesPub, &tmpKvTimesPubv);

    // compute e([π₁]₁, [π₂]₂)
    mclBnGT ePi1Pi2;
    mclBn_millerLoop(&ePi1Pi2, &proof->pi_1, &proof->pi_2);

    // compute e( [Σᵥ (Kᵥ₊₁ * publicInputs[v])]₁, -[γ]₂ )
    mclBnGT eSumKTimesPubGammaNeg;
    mclBn_millerLoop(&eSumKTimesPubGammaNeg, &sumKTimesPub, &vkPrecomputed->gammaNeg);

    // compute e([π₃]₁, -[δ]₂)
    mclBnGT ePi3DeltaNeg;
    mclBn_millerLoop(&ePi3DeltaNeg, &proof->pi_3, &vkPrecomputed->deltaNeg);

    // compute z = e(α, β) * e( [Σᵥ (Kᵥ₊₁ * publicInputs[v])]₁, -[γ]₂ ) * e([π₃]1, -[δ]₂)
    mclBnGT z;
    mclBnGT_mul(&z, &ePi1Pi2, &eSumKTimesPubGammaNeg);
    mclBnGT_mul(&z, &z, &ePi3DeltaNeg);

    // ensure that z is a unique value in GT
    mclBn_finalExp(&z, &z);

    // if e(α, β) * e( [Σᵥ (Kᵥ₊₁ * publicInputs[v])]₁, -[γ]₂ ) * e([π₃]1, -[δ]₂) == e(α, β) then the proof is valid
    return mclBnGT_isEqual(&z, &vkPrecomputed->eAlphaBeta);
}

int verify_groth16_proof(
    const Groth16VerifierKeyInput *vk,
    const Groth16ProofInput *proof,
    const mclBnFr *publicInputs)
{
    Groth16VerifierKeyPrecomputedValues vkPrecomputed;
    precompute_groth16_values(vk, &vkPrecomputed);
    return verify_groth16_proof_precomputed(vk, &vkPrecomputed, proof, publicInputs);
}


*/
using namespace bls12_381;

namespace bls12_381_groth16
{

    bool deserializeG1FromVector(bls12_381::g1 *out, const std::vector<const unsigned char> *v, size_t startIndex)
    {
        if (v->size() < (48 + startIndex))
        {
            return false;
        }
        std::array<unsigned char, 48> s1;
        std::copy(v->begin() + startIndex, v->begin() + startIndex + 48, s1.begin());
        printf("testaa\n");
        printf("hexbytes: %s\n", bytesToHex(s1).c_str());
        auto g1Option = g1::fromCompressedMCLBytesLE(s1);
        if (!g1Option.has_value())
        {
            return false;
        }
        else
        {
            printf("x\n");
            *out = g1Option.value();
            return true;
        }
    }
    bool deserializeG2FromVector(bls12_381::g2 *out, const std::vector<const unsigned char> *v, size_t startIndex)
    {
        if (v->size() < (96 + startIndex))
        {
            return false;
        }
        std::array<unsigned char, 96> s1;
        std::copy(v->begin() + startIndex, v->begin() + startIndex + 96, s1.begin());
        auto g2Option = g2::fromCompressedMCLBytesLE(s1);
        if (!g2Option.has_value())
        {
            return false;
        }
        else
        {
            *out = g2Option.value();
            return true;
        }
    }
    bool deserializeFpFromVector(bls12_381::fp *out, const std::vector<const unsigned char> *v, size_t startIndex)
    {
        if (v->size() < (48 + startIndex))
        {
            return false;
        }
        std::span<const unsigned char, 48> s(v->data() + startIndex, 48);
        auto fpOption = fp::fromBytesLE(s);
        if (!fpOption.has_value())
        {
            return false;
        }
        else
        {
            *out = fpOption.value();
            return true;
        }
    }
    bool deserializeScalarFromVector(std::array<uint64_t, 4> &out, const std::vector<const unsigned char> *v, size_t startIndex)
    {
        if (v->size() < (32 + startIndex))
        {
            return false;
        }
        auto x = (v->data() + startIndex);
        const std::span<const unsigned char, 32> s(x, 32);
        auto k = scalar::fromBytesLE<4>(s);
        out[0] = k[0];
        out[1] = k[1];
        out[2] = k[2];
        out[3] = k[3];
        return true;
    }
    int deserializeProofWith2PublicInputs(
        Groth16ProofWith2PublicInputs *proof,
        const std::vector<unsigned char> *pi_1,
        const std::vector<unsigned char> *pi_2_0,
        const std::vector<unsigned char> *pi_2_1,
        const std::vector<unsigned char> *pi_3,
        const std::vector<unsigned char> *public_input_0,
        const std::vector<unsigned char> *public_input_1)
    {
        if (pi_1->size() != 48 || pi_2_0->size() != 48 || pi_2_1->size() != 48 || pi_3->size() != 48 || public_input_0->size() != 32 || public_input_1->size() != 32)
        {
            return 0;
        }
        if (!deserializeG1FromVector(&proof->pi_1, (const std::vector<const unsigned char> *)pi_1, 0))
        {
            return 0;
        }
        printf("deproof 1\n");

        std::array<unsigned char, 96> pi_2_data;
        std::copy(pi_2_0->begin(), pi_2_0->end(), pi_2_data.begin());
        std::copy(pi_2_1->begin(), pi_2_1->end(), pi_2_data.begin() + 48);
        auto pi_2_v = g2::fromCompressedMCLBytesLE(pi_2_data);
        if (!pi_2_v.has_value())
        {
            return 0;
        }
        proof->pi_2 = pi_2_v.value();
        printf("deproof 2\n");
        if (!deserializeG1FromVector(&proof->pi_3, (const std::vector<const unsigned char> *)pi_3, 0))
        {
            return 0;
        }

        printf("deproof 3\n");
        if (!deserializeScalarFromVector(proof->public_input_0, (const std::vector<const unsigned char> *)public_input_0, 0))
        {
            return 0;
        }
        if (!deserializeScalarFromVector(proof->public_input_1, (const std::vector<const unsigned char> *)public_input_1, 0))
        {
            return 0;
        }
        return 1;
    }

    int deserializeVerifierKeyInput(
        Groth16VerifierKeyInput *vk,
        const std::vector<unsigned char> *a,
        const std::vector<unsigned char> *b,
        const std::vector<unsigned char> *c,
        const std::vector<unsigned char> *d,
        const std::vector<unsigned char> *e,
        const std::vector<unsigned char> *f)
    {
        printf("test111\n");
        if (a->size() != 80 || b->size() != 80 || c->size() != 80 || d->size() != 80 || e->size() != 80 || f->size() != 80)
        {
            return 0;
        }
        printf("dekey 1\n");
        std::array<unsigned char, 480> vkey_tmp_data;
        std::copy(a->begin(), a->end(), vkey_tmp_data.begin());
        std::copy(b->begin(), b->end(), vkey_tmp_data.begin() + 80);
        std::copy(c->begin(), c->end(), vkey_tmp_data.begin() + 160);
        std::copy(d->begin(), d->end(), vkey_tmp_data.begin() + 240);
        std::copy(e->begin(), e->end(), vkey_tmp_data.begin() + 320);
        std::copy(f->begin(), f->end(), vkey_tmp_data.begin() + 400);
        printf("idata1\n");

        std::array<unsigned char, 48> g1_data;
        size_t ptr = 0;
        std::copy(vkey_tmp_data.begin(), vkey_tmp_data.begin() + 48, g1_data.begin());
        printf("idata2\n");
        auto g1_option = g1::fromCompressedMCLBytesLE(g1_data);
        if (!g1_option.has_value())
        {
            return 0;
        }
        vk->alpha = g1_option.value();
        ptr += 48;
        printf("idata3\n");

        std::copy(vkey_tmp_data.begin() + ptr, vkey_tmp_data.begin() + ptr + 48, g1_data.begin());
        g1_option = g1::fromCompressedMCLBytesLE(g1_data);
        if (!g1_option.has_value())
        {
            return 0;
        }
        vk->k[0] = g1_option.value();
        ptr += 48;
        printf("idata4\n");

        std::copy(vkey_tmp_data.begin() + ptr, vkey_tmp_data.begin() + ptr + 48, g1_data.begin());
        g1_option = g1::fromCompressedMCLBytesLE(g1_data);
        if (!g1_option.has_value())
        {
            return 0;
        }
        vk->k[1] = g1_option.value();
        ptr += 48;
        printf("idata5\n");

        std::copy(vkey_tmp_data.begin() + ptr, vkey_tmp_data.begin() + ptr + 48, g1_data.begin());
        g1_option = g1::fromCompressedMCLBytesLE(g1_data);
        if (!g1_option.has_value())
        {
            return 0;
        }
        vk->k[2] = g1_option.value();
        ptr += 48;
        printf("idata6\n");


        std::array<unsigned char, 96> g2_data;
        std::copy(vkey_tmp_data.begin() + ptr, vkey_tmp_data.begin() + ptr + 96, g2_data.begin());
        auto g2_option = g2::fromCompressedMCLBytesLE(g2_data);
        if (!g2_option.has_value())
        {
            return 0;
        }
        vk->beta = g2_option.value();
        ptr += 96;
        printf("idata7\n");

        std::copy(vkey_tmp_data.begin() + ptr, vkey_tmp_data.begin() + ptr + 96, g2_data.begin());
        g2_option = g2::fromCompressedMCLBytesLE(g2_data);
        if (!g2_option.has_value())
        {
            return 0;
        }
        vk->delta = g2_option.value();
        ptr += 96;
        printf("idata8\n");


        std::copy(vkey_tmp_data.begin() + ptr, vkey_tmp_data.begin() + ptr + 96, g2_data.begin());
        g2_option = g2::fromCompressedMCLBytesLE(g2_data);
        if (!g2_option.has_value())
        {
            return 0;
        }
        vk->gamma = g2_option.value();
        ptr += 96;
        printf("idata9\n");
        return 1;
    }
    int precomputeVerifierKey(Groth16VerifierKeyPrecomputedValues *precomputed, const Groth16VerifierKeyInput *vk)
    {
        std::vector<std::tuple<g1, g2>> v;
        pairing::add_pair(v, vk->alpha, vk->beta);
        // pre-compute e(α, β)
        precomputed->eAlphaBeta = pairing::calculate(v);
        precomputed->deltaNeg = vk->delta.negate();
        precomputed->gammaNeg = vk->gamma.negate();
        return 1;
    }
    fp12 computeMillerLoopSingle(g1 a, g2 b) {
        std::vector<std::tuple<g1, g2>> v;
        pairing::add_pair(v, a, b);
        return pairing::miller_loop(v, std::function<void()>());
    }
    int verifyProofWith2PublicInputs(
        const Groth16ProofWith2PublicInputs *proof,
        const Groth16VerifierKeyInput *vk,
        const Groth16VerifierKeyPrecomputedValues *precomputed)
    {
        // [Σᵥ (Kᵥ₊₁ * publicInputs[v])]₁
        g1 sumKTimesPub = vk->k[0];


        //  sumKTimesPub += K₁ * publicInputs[0]
        sumKTimesPub.addAssign(vk->k[1].scale(proof->public_input_0));


        //  sumKTimesPub += K₂ * publicInputs[1]
        sumKTimesPub.addAssign(vk->k[2].scale(proof->public_input_0));

        // compute e([π₁]₁, [π₂]₂)
        fp12 ePi1Pi2 = computeMillerLoopSingle(proof->pi_1, proof->pi_2);

        // compute e( [Σᵥ (Kᵥ₊₁ * publicInputs[v])]₁, -[γ]₂ )
        fp12 eSumKTimesPubGammaNeg = computeMillerLoopSingle(sumKTimesPub, precomputed->gammaNeg);

        // compute e([π₃]₁, -[δ]₂)
        fp12 ePi3DeltaNeg = computeMillerLoopSingle(proof->pi_3, precomputed->deltaNeg);

        // compute z = e(α, β) * e( [Σᵥ (Kᵥ₊₁ * publicInputs[v])]₁, -[γ]₂ ) * e([π₃]1, -[δ]₂)
        fp12 z = ePi1Pi2.multiply(eSumKTimesPubGammaNeg).multiply(ePi3DeltaNeg);
        pairing::final_exponentiation(z);
        if(z.equal(precomputed->eAlphaBeta)){
            return 1;
        }else{
            return 0;
        }
    }
    
}